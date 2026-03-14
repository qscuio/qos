from __future__ import annotations

from pathlib import Path

from orchestrator.core.image_assets import release_asset_paths, resolve_mirror_url
from orchestrator.core.stages import StageDefinition, make_stage_result


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _execute(request, manifests, request_root: Path) -> dict:
    assets = release_asset_paths(LINUX_LAB_ROOT, request.image_release)
    image_path = Path(request.artifact_root) / "images" / f"{request.image_release}.img"
    mirror_url = resolve_mirror_url(
        image_release=request.image_release,
        arch=request.arch,
        region=request.mirror_region,
    )
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"

    log_path = request_root / "logs" / "build-image.log"
    log_path.write_text("build-image: runtime metadata emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="build-image",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "kernel-plan", "tools-plan", "examples-plan"),
        outputs=("image-plan",),
        log_path=log_path,
        metadata={
            "image_path": str(image_path),
            "release_dir": str(assets["release_dir"]),
            "netcfg_path": str(assets["netcfg_path"]),
            "sources_list_path": str(assets["sources_list_path"]),
            "mirror_url": mirror_url,
            "commands": [
                [
                    "debootstrap",
                    f"--arch={request.arch}",
                    request.image_release,
                    str(Path(request.artifact_root) / "chroot"),
                    mirror_url or "default-mirror",
                ]
            ],
        },
    )


STAGE = StageDefinition(
    name="build-image",
    depends_on=("build-kernel", "build-tools", "build-examples"),
    consumes=("resolved-request", "kernel-plan", "tools-plan", "examples-plan"),
    produces=("image-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
