from __future__ import annotations

import os
import shutil
from pathlib import Path

from orchestrator.core.examples import resolve_example_plan
from orchestrator.core.stages import StageDefinition, auto_jobs, make_stage_result, run_stage_command


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _linux_lab_root() -> Path:
    override = os.environ.get("LINUX_LAB_ROOT_OVERRIDE")
    if override:
        return Path(override)
    return LINUX_LAB_ROOT


def _resolve_example_groups(manifests, request) -> list[str]:
    if "minimal" in request.profiles:
        return []
    groups: list[str] = []
    for profile_key in request.profiles:
        for group in manifests.profiles[profile_key].example_groups:
            if group not in groups:
                groups.append(group)
    return groups


def _write_or_replace_block(path: Path, block_id: str, content: str) -> None:
    begin = f"# linux-lab:{block_id}:begin"
    end = f"# linux-lab:{block_id}:end"
    current = path.read_text(encoding="utf-8")
    block = f"{begin}\n{content.rstrip()}\n{end}\n"
    if begin in current and end in current:
        prefix, remainder = current.split(begin, 1)
        _old, suffix = remainder.split(end, 1)
        new_text = f"{prefix}{block}{suffix.lstrip(chr(10))}"
    else:
        separator = "" if current.endswith("\n") else "\n"
        new_text = f"{current}{separator}{block}"
    path.write_text(new_text, encoding="utf-8")


def _stage_rust_samples(request, manifests, request_root: Path) -> dict:
    example_groups = _resolve_example_groups(manifests, request)
    if not any(group.startswith("rust-") for group in example_groups):
        return {
            "staged_rust_samples": [],
            "rust_kconfig_path": None,
            "rust_makefile_path": None,
        }

    linux_lab_root = _linux_lab_root()
    arch = manifests.arches[request.arch]
    kernel_tree = request_root / "workspace" / "kernel" / f"linux-{request.kernel_version}"
    example_plans = resolve_example_plan(
        example_groups=example_groups,
        linux_lab_root=linux_lab_root,
        build_root=request_root / "workspace" / "build",
        kernel_tree=kernel_tree,
        arch=request.arch,
        toolchain_prefix=arch.toolchain_prefix,
        kernel_version=request.kernel_version,
    )
    rust_plans = [plan for plan in example_plans if plan["group"].startswith("rust-")]
    sample_sources = sorted(
        {
            Path(path)
            for plan in rust_plans
            for path in plan.get("kernel_sample_sources", [])
        },
        key=lambda path: path.name,
    )
    if not sample_sources:
        return {
            "staged_rust_samples": [],
            "rust_kconfig_path": None,
            "rust_makefile_path": None,
        }

    samples_rust = kernel_tree / "samples" / "rust"
    kconfig_path = samples_rust / "Kconfig"
    makefile_path = samples_rust / "Makefile"

    source_roots = sorted({path.parent for path in sample_sources}, key=lambda path: str(path))
    kconfig_blocks: list[str] = []
    makefile_blocks: list[str] = []
    for root in source_roots:
        source_kconfig = root / "Kconfig"
        source_makefile = root / "Makefile"
        if source_kconfig.exists():
            kconfig_blocks.append(source_kconfig.read_text(encoding="utf-8").rstrip())
        if source_makefile.exists():
            makefile_blocks.append(source_makefile.read_text(encoding="utf-8").rstrip())

    if request.command == "run" and not request.dry_run:
        samples_rust.mkdir(parents=True, exist_ok=True)
        for source in sample_sources:
            shutil.copy2(source, samples_rust / source.name)
        if kconfig_path.exists() and kconfig_blocks:
            _write_or_replace_block(kconfig_path, "rust-learn-kconfig", "\n\n".join(kconfig_blocks))
        if makefile_path.exists() and makefile_blocks:
            _write_or_replace_block(makefile_path, "rust-learn-makefile", "\n".join(makefile_blocks))

    return {
        "staged_rust_samples": [path.name for path in sample_sources],
        "rust_kconfig_path": str(kconfig_path),
        "rust_makefile_path": str(makefile_path),
    }


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    arch = manifests.arches[request.arch]
    workspace_root = request_root / "workspace"
    kernel_tree = workspace_root / "kernel" / f"linux-{request.kernel_version}"
    build_root = workspace_root / "build"
    kernel_image_path = build_root / "arch" / request.arch / "boot" / arch.kernel_image_name
    jobs = auto_jobs()
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
    elif request.command == "run":
        status = "succeeded"

    rust_stage = _stage_rust_samples(request, manifests, request_root)

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
            jobs,
        ],
        [
            "make",
            f"O={build_root}",
            f"ARCH={request.arch}",
            f"CROSS_COMPILE={arch.toolchain_prefix}",
            "modules",
            "-j",
            jobs,
        ],
    ]

    log_path = request_root / "logs" / "build-kernel.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    if request.command == "run" and not request.dry_run:
        log_path.write_text("", encoding="utf-8")
        for command in commands:
            run_stage_command(command, cwd=kernel_tree, log_path=log_path)
    else:
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
            **rust_stage,
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
