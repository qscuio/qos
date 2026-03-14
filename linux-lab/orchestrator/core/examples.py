from __future__ import annotations

from pathlib import Path
from typing import Any

from orchestrator.core.example_catalog import load_example_catalog


EXAMPLE_GROUP_ORDER = ["modules-core", "userspace-core", "rust-core", "bpf-core"]
GROUP_KIND_MAP = {
    "modules-core": "module",
    "userspace-core": "userspace",
    "rust-core": "rust",
    "bpf-core": "bpf",
}
GROUP_ENTRY_ORDER = {
    "modules-core": ["modules-debug", "simple", "ioctl"],
    "userspace-core": ["userspace-app"],
    "rust-core": ["rust_learn"],
    "bpf-core": ["bpf-learn"],
}


def _enabled_entries_for_group(linux_lab_root: Path, group: str) -> list[dict[str, Any]]:
    catalog = load_example_catalog(linux_lab_root / "catalog" / "examples")
    expected_keys = GROUP_ENTRY_ORDER[group]
    entries = [
        catalog[key]
        for key in expected_keys
        if key in catalog and catalog[key]["enabled"] and catalog[key]["kind"] == GROUP_KIND_MAP[group]
    ]
    return entries


def _resolve_catalog_source(linux_lab_root: Path, source: str) -> Path:
    source_path = Path(source)
    repo_relative = linux_lab_root.parent / source_path
    if repo_relative.exists():
        return repo_relative
    if source_path.parts and source_path.parts[0] == "linux-lab":
        source_path = Path(*source_path.parts[1:])
    return linux_lab_root / source_path


def _modules_plan(linux_lab_root: Path, build_root: Path) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, "modules-core")
    module_dirs = [_resolve_catalog_source(linux_lab_root, entry["source"]) for entry in entries]
    commands = [["make", "-C", str(build_root), f"M={path}"] for path in module_dirs]
    return {
        "group": "modules-core",
        "entries": entries,
        "paths": [str(path) for path in module_dirs],
        "commands": commands,
    }


def _userspace_plan(linux_lab_root: Path) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, "userspace-core")
    app_dir = _resolve_catalog_source(linux_lab_root, entries[0]["source"])
    sources = [app_dir / "poll.c", app_dir / "char_file.c", app_dir / "netlink_demo.c"]
    commands = [
        ["gcc", "-g", "-O0", "-Wall", "-Wextra", "-o", str(path.with_suffix("")), str(path)]
        for path in sources
    ]
    return {
        "group": "userspace-core",
        "entries": entries,
        "paths": [str(path) for path in sources],
        "commands": commands,
    }


def _rust_plan(linux_lab_root: Path) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, "rust-core")
    rust_dir = _resolve_catalog_source(linux_lab_root, entries[0]["source"])
    user_dir = rust_dir / "user"
    return {
        "group": "rust-core",
        "entries": entries,
        "paths": [str(rust_dir), str(user_dir)],
        "commands": [["make", "-C", str(user_dir)]],
        "kernel_sample_sources": [
            str(rust_dir / "hello_rust.rs"),
            str(rust_dir / "rust_chardev.rs"),
        ],
    }


def _bpf_plan(linux_lab_root: Path) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, "bpf-core")
    bpf_dir = _resolve_catalog_source(linux_lab_root, entries[0]["source"])
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
        "entries": entries,
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
