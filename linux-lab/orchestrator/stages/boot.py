from __future__ import annotations

import os
from pathlib import Path

from orchestrator.core.qemu import build_qemu_command
from orchestrator.core.stages import StageDefinition, make_stage_result, run_stage_command


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _linux_lab_root() -> Path:
    override = os.environ.get("LINUX_LAB_ROOT_OVERRIDE")
    if override:
        return Path(override)
    return LINUX_LAB_ROOT


def _execute(request, manifests, request_root: Path) -> dict:
    linux_lab_root = _linux_lab_root()
    arch = manifests.arches[request.arch]
    kernel_image = (
        request_root
        / "workspace"
        / "build"
        / "arch"
        / request.arch
        / "boot"
        / arch.kernel_image_name
    )
    image_path = request_root / "images" / f"{request.image_release}.img"
    script_dir = linux_lab_root / "scripts"
    boot_script = script_dir / "boot.sh"
    up_script = script_dir / "up.sh"
    down_script = script_dir / "down.sh"
    pidfile = request_root / "runtime" / "qemu.pid"
    qemu_command = build_qemu_command(
        arch=request.arch,
        kernel_image=kernel_image,
        disk_image=image_path,
        shared_dir=linux_lab_root.parent,
        network_up_script=up_script,
        network_down_script=down_script,
        pidfile=pidfile,
        qemu_cpu=getattr(arch, "qemu_cpu", None),
    )
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"

    log_path = request_root / "logs" / "boot.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.write_text("boot: runtime metadata emitted\n", encoding="utf-8")
    command = [str(boot_script), str(log_path), str(pidfile), *qemu_command]
    if request.command == "run" and not request.dry_run:
        pidfile.parent.mkdir(parents=True, exist_ok=True)
        run_stage_command(command, cwd=linux_lab_root.parent, log_path=log_path)
        status = "succeeded"
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
            "script_path": str(boot_script),
            "helper_scripts": {
                "up": str(up_script),
                "down": str(down_script),
                "connect": str(script_dir / "connect"),
                "gdb": str(script_dir / "gdb"),
            },
            "command": command,
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
