from __future__ import annotations

from pathlib import Path

from orchestrator.core.stages import StageDefinition, make_stage_result


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    arch = manifests.arches[request.arch]
    workspace_root = Path(request.artifact_root) / "workspace"
    kernel_tree = workspace_root / "kernel" / f"linux-{request.kernel_version}"
    build_root = workspace_root / "build"
    kernel_image_path = build_root / "arch" / request.arch / "boot" / arch.kernel_image_name
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"

    commands = [
        [
            "make",
            f"O={build_root}",
            f"ARCH={request.arch}",
            f"CROSS_COMPILE={arch.toolchain_prefix}",
            "olddefconfig",
        ],
        [
            "make",
            f"O={build_root}",
            f"ARCH={request.arch}",
            f"CROSS_COMPILE={arch.toolchain_prefix}",
            arch.kernel_image_name,
            "-j",
            "auto",
        ],
        [
            "make",
            f"O={build_root}",
            f"ARCH={request.arch}",
            f"CROSS_COMPILE={arch.toolchain_prefix}",
            "modules",
            "-j",
            "auto",
        ],
    ]

    log_path = request_root / "logs" / "build-kernel.log"
    log_path.write_text("build-kernel: runtime metadata emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="build-kernel",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "config-plan"),
        outputs=("kernel-plan",),
        log_path=log_path,
        metadata={
            "workspace_root": str(workspace_root),
            "kernel_tree": str(kernel_tree),
            "build_root": str(build_root),
            "kernel_image_name": arch.kernel_image_name,
            "kernel_image_path": str(kernel_image_path),
            "commands": commands,
            "config_family": kernel.config_family,
        },
    )


STAGE = StageDefinition(
    name="build-kernel",
    depends_on=("configure",),
    consumes=("resolved-request", "config-plan"),
    produces=("kernel-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
