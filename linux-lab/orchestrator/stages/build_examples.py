from __future__ import annotations

import os
from pathlib import Path

from orchestrator.core.examples import resolve_example_plan
from orchestrator.core.stages import StageDefinition, make_stage_result, run_stage_command


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _linux_lab_root() -> Path:
    override = os.environ.get("LINUX_LAB_ROOT_OVERRIDE")
    if override:
        return Path(override)
    return LINUX_LAB_ROOT


def _resolve_example_groups(manifests, request) -> list[str]:
    if "minimal" in request.profiles:
        return []
    groups: list[str] = []
    for profile_key in request.profiles:
        for group in manifests.profiles[profile_key].example_groups:
            if group not in groups:
                groups.append(group)
    return groups


def _execute(request, manifests, request_root: Path) -> dict:
    example_groups = _resolve_example_groups(manifests, request)
    arch_manifest = manifests.arches[request.arch]
    example_plans = resolve_example_plan(
        example_groups=example_groups,
        linux_lab_root=_linux_lab_root(),
        build_root=request_root / "workspace" / "build",
        kernel_tree=request_root / "workspace" / "kernel" / f"linux-{request.kernel_version}",
        arch=request.arch,
        toolchain_prefix=arch_manifest.toolchain_prefix,
        kernel_version=request.kernel_version,
    )
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
    log_path = request_root / "logs" / "build-examples.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.write_text("build-examples: runtime metadata emitted\n", encoding="utf-8")
    if request.command == "run" and not request.dry_run:
        for plan in example_plans:
            for command in plan["commands"]:
                run_stage_command(command, cwd=request_root, log_path=log_path)
        status = "succeeded"
    return make_stage_result(
        stage="build-examples",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "kernel-plan", "tools-plan"),
        outputs=("examples-plan",),
        log_path=log_path,
        metadata={
            "example_groups": example_groups,
            "example_plans": example_plans,
        },
    )


STAGE = StageDefinition(
    name="build-examples",
    depends_on=("build-kernel", "build-tools"),
    consumes=("resolved-request", "kernel-plan", "tools-plan"),
    produces=("examples-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
