from __future__ import annotations

from pathlib import Path

from orchestrator.core.examples import resolve_example_plan
from orchestrator.core.stages import StageDefinition, make_stage_result


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _resolve_example_groups(manifests, request) -> list[str]:
    kernel = manifests.kernels[request.kernel_version]
    groups: list[str] = []
    for group in kernel.example_groups:
        if group not in groups:
            groups.append(group)
    for profile_key in request.profiles:
        for group in manifests.profiles[profile_key].example_groups:
            if group not in groups:
                groups.append(group)
    return groups


def _execute(request, manifests, request_root: Path) -> dict:
    example_groups = _resolve_example_groups(manifests, request)
    example_plans = resolve_example_plan(
        example_groups=example_groups,
        linux_lab_root=LINUX_LAB_ROOT,
        build_root=Path(request.artifact_root) / "workspace" / "build",
    )
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
    log_path = request_root / "logs" / "build-examples.log"
    log_path.write_text("build-examples: runtime metadata emitted\n", encoding="utf-8")
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
