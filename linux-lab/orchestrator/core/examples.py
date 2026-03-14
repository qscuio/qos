from __future__ import annotations

from pathlib import Path
from typing import Any


EXAMPLE_GROUP_ORDER = ["modules-core", "userspace-core", "rust-core", "bpf-core"]


def _modules_plan(linux_lab_root: Path, build_root: Path) -> dict[str, Any]:
    module_dirs = [
        linux_lab_root / "examples" / "modules" / "debug",
        linux_lab_root / "examples" / "modules" / "simple",
        linux_lab_root / "examples" / "modules" / "ioctl",
    ]
    commands = [["make", "-C", str(build_root), f"M={path}"] for path in module_dirs]
    return {
        "group": "modules-core",
        "paths": [str(path) for path in module_dirs],
        "commands": commands,
    }


def _userspace_plan(linux_lab_root: Path) -> dict[str, Any]:
    app_dir = linux_lab_root / "examples" / "userspace" / "app"
    sources = [app_dir / "poll.c", app_dir / "char_file.c", app_dir / "netlink_demo.c"]
    commands = [
        ["gcc", "-g", "-O0", "-Wall", "-Wextra", "-o", str(path.with_suffix("")), str(path)]
        for path in sources
    ]
    return {
        "group": "userspace-core",
        "paths": [str(path) for path in sources],
        "commands": commands,
    }


def _rust_plan(linux_lab_root: Path) -> dict[str, Any]:
    rust_dir = linux_lab_root / "examples" / "rust" / "rust_learn"
    user_dir = rust_dir / "user"
    return {
        "group": "rust-core",
        "paths": [str(rust_dir), str(user_dir)],
        "commands": [["make", "-C", str(user_dir)]],
        "kernel_sample_sources": [
            str(rust_dir / "hello_rust.rs"),
            str(rust_dir / "rust_chardev.rs"),
        ],
    }


def _bpf_plan(linux_lab_root: Path) -> dict[str, Any]:
    bpf_dir = linux_lab_root / "examples" / "bpf" / "learn"
    sources = [
        bpf_dir / "hello.c",
        bpf_dir / "packet_filter.c",
        bpf_dir / "tracing.c",
        bpf_dir / "xdp.c",
        bpf_dir / "open.c",
    ]
    commands = [
        ["clang", "-O2", "-g", "-target", "bpf", "-c", str(path), "-o", str(path.with_suffix(".o"))]
        for path in sources
    ]
    return {
        "group": "bpf-core",
        "paths": [str(path) for path in sources],
        "commands": commands,
    }


def resolve_example_plan(*, example_groups: list[str], linux_lab_root: Path, build_root: Path) -> list[dict[str, Any]]:
    planners = {
        "modules-core": lambda: _modules_plan(linux_lab_root, build_root),
        "userspace-core": lambda: _userspace_plan(linux_lab_root),
        "rust-core": lambda: _rust_plan(linux_lab_root),
        "bpf-core": lambda: _bpf_plan(linux_lab_root),
    }

    resolved: list[dict[str, Any]] = []
    for group in EXAMPLE_GROUP_ORDER:
        if group in example_groups:
            resolved.append(planners[group]())
    return resolved
