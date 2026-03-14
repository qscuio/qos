from __future__ import annotations

from pathlib import Path

from orchestrator.core.kernel_assets import merge_kernel_config, resolve_profile_fragment_paths
from orchestrator.core.stages import StageDefinition, make_stage_result


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _display_ref(path: Path) -> str:
    repo_root = LINUX_LAB_ROOT.parent
    if path.is_absolute() and path.is_relative_to(repo_root):
        return str(path.relative_to(repo_root))
    return str(path)


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    base_config = LINUX_LAB_ROOT / "configs" / request.arch / "defconfig"
    fragment_paths = resolve_profile_fragment_paths(manifests, request.profiles, LINUX_LAB_ROOT)
    fragment_refs = [_display_ref(path) for path in fragment_paths]
    output_config = Path(request.artifact_root) / "workspace" / "build" / ".config"
    log_path = request_root / "logs" / "configure.log"
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text("configure: dry-run config planning emitted\n", encoding="utf-8")
    elif request.command == "run":
        merge_kernel_config(base_config, fragment_paths, output_config)
        status = "succeeded"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text(f"configure: wrote {output_config}\n", encoding="utf-8")
    else:
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text("configure: placeholder config planning emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="configure",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "patch-plan"),
        outputs=("config-plan",),
        log_path=log_path,
        metadata={
            "config_family": kernel.config_family,
            "base_config": _display_ref(base_config),
            "fragment_refs": fragment_refs,
            "output_config": str(output_config),
        },
    )


STAGE = StageDefinition(
    name="configure",
    depends_on=("patch",),
    consumes=("resolved-request", "patch-plan"),
    produces=("config-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
