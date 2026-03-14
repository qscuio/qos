from __future__ import annotations

import os
from pathlib import Path

from orchestrator.core.image_assets import release_asset_paths, resolve_mirror_url
from orchestrator.core.stages import StageDefinition, make_stage_result, run_stage_command


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _linux_lab_root() -> Path:
    override = os.environ.get("LINUX_LAB_ROOT_OVERRIDE")
    if override:
        return Path(override)
    return LINUX_LAB_ROOT


def _execute(request, manifests, request_root: Path) -> dict:
    linux_lab_root = _linux_lab_root()
    assets = release_asset_paths(linux_lab_root, request.image_release)
    script_path = linux_lab_root / "scripts" / "create-image.sh"
    image_path = Path(request.artifact_root) / "images" / f"{request.image_release}.img"
    chroot_dir = Path(request.artifact_root) / "chroot"
    mirror_url = resolve_mirror_url(
        image_release=request.image_release,
        arch=request.arch,
        region=request.mirror_region,
    )
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"

    command = [
        str(script_path),
        "--arch",
        request.arch,
        "--release",
        request.image_release,
        "--image",
        str(image_path),
        "--chroot",
        str(chroot_dir),
        "--netcfg",
        str(assets["netcfg_path"]),
        "--sources-list",
        str(assets["sources_list_path"]),
    ]
    if mirror_url is not None:
        command.extend(["--mirror-url", mirror_url])

    log_path = request_root / "logs" / "build-image.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.write_text("build-image: runtime metadata emitted\n", encoding="utf-8")
    if request.command == "run" and not request.dry_run:
        run_stage_command(command, cwd=linux_lab_root, log_path=log_path)
        status = "succeeded"
    return make_stage_result(
        stage="build-image",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "kernel-plan", "tools-plan", "examples-plan"),
        outputs=("image-plan",),
        log_path=log_path,
        metadata={
            "script_path": str(script_path),
            "image_path": str(image_path),
            "chroot_dir": str(chroot_dir),
            "release_dir": str(assets["release_dir"]),
            "netcfg_path": str(assets["netcfg_path"]),
            "sources_list_path": str(assets["sources_list_path"]),
            "mirror_url": mirror_url,
            "commands": [command],
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
