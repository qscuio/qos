from __future__ import annotations

from pathlib import Path

from orchestrator.core.kernel_assets import apply_kernel_patch, extract_kernel_archive
from orchestrator.core.stages import StageDefinition, make_stage_result


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    log_path = request_root / "logs" / "patch.log"
    workspace_root = Path(request.artifact_root) / "workspace"
    source_tree = workspace_root / "kernel" / f"linux-{request.kernel_version}"
    patch_path = None
    if kernel.patch_set:
        patch_path = Path(kernel.patch_set)
        if not patch_path.is_absolute():
            patch_path = LINUX_LAB_ROOT.parent / patch_path

    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text("patch: dry-run patch metadata emitted\n", encoding="utf-8")
    elif request.command == "run":
        archive_path = request_root.parents[1] / "cache" / f"linux-{request.kernel_version}.tar.xz"
        source_tree = extract_kernel_archive(
            version=request.kernel_version,
            archive_path=archive_path,
            workspace_root=workspace_root,
        )
        if patch_path is not None:
            apply_kernel_patch(workspace_root=workspace_root, patch_path=patch_path)
        status = "succeeded"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text(f"patch: prepared source tree {source_tree}\n", encoding="utf-8")
    else:
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text("patch: placeholder patch metadata emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="patch",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "source-plan", "prepare-report"),
        outputs=("patch-plan",),
        log_path=log_path,
        metadata={
            "patch_path": kernel.patch_set,
            "workspace_root": str(workspace_root),
            "source_tree": str(source_tree),
            "applied_patch": patch_path is not None,
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
