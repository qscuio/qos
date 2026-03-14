from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from orchestrator.core.state import ensure_request_dirs, iso_now, touch_log, write_json


StageExecutor = Callable[[object, object, Path], dict]


@dataclass(frozen=True)
class StageDefinition:
    name: str
    depends_on: tuple[str, ...]
    consumes: tuple[str, ...]
    produces: tuple[str, ...]
    supports_delegate: bool
    supports_placeholder: bool
    executor: StageExecutor


def make_stage_result(
    *,
    stage: str,
    status: str,
    request_fingerprint: str,
    inputs: tuple[str, ...],
    outputs: tuple[str, ...],
    log_path: Path,
    metadata: dict | None = None,
    error_kind: str | None = None,
    error_message: str | None = None,
) -> dict:
    timestamp = iso_now()
    result = {
        "stage": stage,
        "status": status,
        "started_at": timestamp,
        "ended_at": timestamp,
        "request_fingerprint": request_fingerprint,
        "inputs": list(inputs),
        "outputs": list(outputs),
        "log_path": str(log_path),
        "error_kind": error_kind,
        "error_message": error_message,
    }
    if metadata is not None:
        result["metadata"] = metadata
    return result


def placeholder_executor(stage_name: str) -> StageExecutor:
    def _execute(request, manifests, request_root: Path) -> dict:
        log_path = request_root / "logs" / f"{stage_name}.log"
        touch_log(log_path, f"{stage_name}: placeholder planning emitted")
        return make_stage_result(
            stage=stage_name,
            status="placeholder",
            request_fingerprint=request.request_fingerprint,
            inputs=(),
            outputs=(),
            log_path=log_path,
        )

    return _execute


def validate_state(request, request_root: Path) -> dict:
    log_path = request_root / "logs" / "validate.log"
    touch_log(log_path, "validate: static request resolution succeeded")
    return make_stage_result(
        stage="validate",
        status="succeeded",
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request",),
        outputs=("resolved-request",),
        log_path=log_path,
    )


def prepare_executor(stage_name: str) -> StageExecutor:
    def _execute(request, manifests, request_root: Path) -> dict:
        if sys.version_info < (3, 12):
            raise RuntimeError("python 3.12 or newer is required")
        probe = request_root / "logs" / ".write-probe"
        probe.write_text("ok\n", encoding="utf-8")
        probe.unlink()
        log_path = request_root / "logs" / f"{stage_name}.log"
        touch_log(log_path, "prepare: writable request root and supported python runtime")
        return make_stage_result(
            stage=stage_name,
            status="succeeded",
            request_fingerprint=request.request_fingerprint,
            inputs=("resolved-request", "source-plan"),
            outputs=("prepare-report",),
            log_path=log_path,
        )

    return _execute


def plan_result_executor(stage_name: str, inputs: tuple[str, ...], outputs: tuple[str, ...], log_line: str) -> StageExecutor:
    def _execute(request, manifests, request_root: Path) -> dict:
        log_path = request_root / "logs" / f"{stage_name}.log"
        touch_log(log_path, log_line)
        return make_stage_result(
            stage=stage_name,
            status="placeholder",
            request_fingerprint=request.request_fingerprint,
            inputs=inputs,
            outputs=outputs,
            log_path=log_path,
        )

    return _execute


def write_stage_state(request_root: Path, result: dict) -> None:
    write_json(request_root / "state" / f"{result['stage']}.json", result)


def execute_plan(request, manifests, stage_definitions: list[StageDefinition]) -> None:
    request_root = Path(request.artifact_root)
    ensure_request_dirs(request_root)

    validate_result = validate_state(request, request_root)
    write_stage_state(request_root, validate_result)

    for stage in stage_definitions:
        result = stage.executor(request, manifests, request_root)
        if not result["inputs"]:
            result["inputs"] = list(stage.consumes)
        if not result["outputs"]:
            result["outputs"] = list(stage.produces)
        write_stage_state(request_root, result)
