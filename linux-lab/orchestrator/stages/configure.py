from __future__ import annotations

from pathlib import Path

from orchestrator.core.kernel_assets import resolve_profile_fragment_paths
from orchestrator.core.stages import StageDefinition, make_stage_result


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    base_config = Path("linux-lab") / "configs" / request.arch / "defconfig"
    fragment_paths = resolve_profile_fragment_paths(manifests, request.profiles, LINUX_LAB_ROOT)
    fragment_refs = [str(path.relative_to(LINUX_LAB_ROOT.parent)) for path in fragment_paths]
    log_path = request_root / "logs" / "configure.log"
    log_path.write_text("configure: placeholder config planning emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="configure",
        status="placeholder",
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "patch-plan"),
        outputs=("config-plan",),
        log_path=log_path,
        metadata={
            "config_family": kernel.config_family,
            "base_config": str(base_config),
            "fragment_refs": fragment_refs,
            "output_config": str(Path(request.artifact_root) / "workspace" / ".config"),
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
