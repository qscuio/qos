from __future__ import annotations

from pathlib import Path

from orchestrator.core.stages import StageDefinition, make_stage_result


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    log_path = request_root / "logs" / "patch.log"
    log_path.write_text("patch: placeholder patch metadata emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="patch",
        status="placeholder",
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "source-plan", "prepare-report"),
        outputs=("patch-plan",),
        log_path=log_path,
        metadata={
            "patch_path": kernel.patch_set,
            "workspace_root": str(Path(request.artifact_root) / "workspace"),
        },
    )


STAGE = StageDefinition(
    name="patch",
    depends_on=("prepare",),
    consumes=("resolved-request", "source-plan", "prepare-report"),
    produces=("patch-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
