from __future__ import annotations

from pathlib import Path

from orchestrator.core.qemu import build_qemu_command
from orchestrator.core.stages import StageDefinition, make_stage_result


def _execute(request, manifests, request_root: Path) -> dict:
    arch = manifests.arches[request.arch]
    kernel_image = (
        Path(request.artifact_root)
        / "workspace"
        / "build"
        / "arch"
        / request.arch
        / "boot"
        / arch.kernel_image_name
    )
    image_path = Path(request.artifact_root) / "images" / f"{request.image_release}.img"
    qemu_command = build_qemu_command(
        arch=request.arch,
        kernel_image=kernel_image,
        disk_image=image_path,
        shared_dir=Path("."),
    )
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"

    log_path = request_root / "logs" / "boot.log"
    log_path.write_text("boot: runtime metadata emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="boot",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "image-plan"),
        outputs=("boot-plan",),
        log_path=log_path,
        metadata={
            "kernel_image": str(kernel_image),
            "disk_image": str(image_path),
            "qemu_command": qemu_command,
        },
    )


STAGE = StageDefinition(
    name="boot",
    depends_on=("build-image",),
    consumes=("resolved-request", "image-plan"),
    produces=("boot-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
