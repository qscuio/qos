from __future__ import annotations

from pathlib import Path
from typing import Any

from orchestrator.core.example_catalog import load_example_catalog


EXAMPLE_GROUP_ORDER = [
    "modules-core",
    "userspace-core",
    "rust-core",
    "bpf-core",
    "modules-all",
    "userspace-all",
    "rust-all",
    "bpf-all",
]
GROUP_KIND_MAP = {
    "modules-core": "module",
    "userspace-core": "userspace",
    "rust-core": "rust",
    "bpf-core": "bpf",
    "modules-all": "module",
    "userspace-all": "userspace",
    "rust-all": "rust",
    "bpf-all": "bpf",
}
GROUP_ENTRY_PRIORITY = {
    "modules-core": ["modules-debug", "simple", "ioctl"],
    "userspace-core": ["userspace-app"],
    "rust-core": ["rust_learn"],
    "bpf-core": ["bpf-learn"],
    "modules-all": ["modules-debug", "simple", "ioctl"],
    "userspace-all": ["userspace-app"],
    "rust-all": ["rust_learn"],
    "bpf-all": ["bpf-learn"],
}


def _entry_matches_group(item: dict[str, Any], group: str) -> bool:
    groups = item.get("groups")
    if not groups:
        return True
    return group in groups


def _enabled_entries_for_group(linux_lab_root: Path, group: str) -> list[dict[str, Any]]:
    catalog = load_example_catalog(linux_lab_root / "catalog" / "examples")
    kind = GROUP_KIND_MAP[group]
    priority = {key: index for index, key in enumerate(GROUP_ENTRY_PRIORITY.get(group, []))}
    entries = [
        item
        for item in catalog.values()
        if item["enabled"] and item["kind"] == kind and _entry_matches_group(item, group)
    ]
    entries.sort(key=lambda item: (priority.get(item["key"], len(priority)), item["key"]))
    return entries


def _resolve_catalog_source(linux_lab_root: Path, source: str) -> Path:
    source_path = Path(source)
    repo_relative = linux_lab_root.parent / source_path
    if repo_relative.exists():
        return repo_relative
    if source_path.parts and source_path.parts[0] == "linux-lab":
        source_path = Path(*source_path.parts[1:])
    return linux_lab_root / source_path


def _command_workdir(base_path: Path, entry: dict[str, Any]) -> Path:
    build_workdir = entry.get("build_workdir")
    if build_workdir:
        return base_path / build_workdir
    return base_path


def _format_command(command: list[str], context: dict[str, str]) -> list[str]:
    return [part.format(**context) for part in command]


def _catalog_commands(
    entry: dict[str, Any],
    source_path: Path,
    build_root: Path,
    *,
    kernel_tree: Path | None = None,
    arch: str = "",
    toolchain_prefix: str = "",
    kernel_version: str = "",
) -> list[list[str]]:
    context = {
        "entry_root": str(source_path),
        "build_root": str(build_root),
        "kernel_tree": str(kernel_tree or build_root),
        "arch": arch,
        "toolchain_prefix": toolchain_prefix,
        "kernel_version": kernel_version,
    }
    if entry.get("build_commands"):
        return [_format_command(command, context) for command in entry["build_commands"]]
    build_mode = entry["build_mode"]
    if build_mode == "kbuild-module":
        return [["make", "-C", str(build_root), f"M={source_path}"]]
    if build_mode == "gcc-userspace":
        return [
            ["gcc", "-g", "-O0", "-Wall", "-Wextra", "-o", str(path.with_suffix("")), str(path)]
            for path in sorted(source_path.glob("*.c"))
        ]
    if build_mode == "rust-user":
        workdir = _command_workdir(source_path, entry)
        if "build_workdir" not in entry and (source_path / "user" / "Makefile").exists():
            workdir = source_path / "user"
        return [["make", "-C", str(workdir)]]
    if build_mode == "bpf-clang":
        return [
            ["clang", "-O2", "-g", "-target", "bpf", "-c", str(path), "-o", str(path.with_suffix(".o"))]
            for path in sorted(source_path.glob("*.c"))
        ]
    if build_mode == "kernel-tree-rust":
        return []
    if build_mode == "custom-make":
        return [["make", "-C", str(_command_workdir(source_path, entry))]]
    if build_mode == "disabled":
        return []
    raise ValueError(f"unsupported build_mode in example planner: {build_mode}")


def _modules_plan(
    group: str,
    linux_lab_root: Path,
    build_root: Path,
    *,
    kernel_tree: Path | None = None,
    arch: str = "",
    toolchain_prefix: str = "",
    kernel_version: str = "",
) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, group)
    module_dirs = [_resolve_catalog_source(linux_lab_root, entry["source"]) for entry in entries]
    commands: list[list[str]] = []
    for entry, path in zip(entries, module_dirs, strict=True):
        commands.extend(
            _catalog_commands(
                entry,
                path,
                build_root,
                kernel_tree=kernel_tree,
                arch=arch,
                toolchain_prefix=toolchain_prefix,
                kernel_version=kernel_version,
            )
        )
    return {
        "group": group,
        "entries": entries,
        "paths": [str(path) for path in module_dirs],
        "commands": commands,
    }


def _userspace_plan(
    group: str,
    linux_lab_root: Path,
    *,
    build_root: Path,
    kernel_tree: Path | None = None,
    arch: str = "",
    toolchain_prefix: str = "",
    kernel_version: str = "",
) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, group)
    sources: list[Path] = []
    commands: list[list[str]] = []
    for entry in entries:
        app_dir = _resolve_catalog_source(linux_lab_root, entry["source"])
        sources.extend(sorted(app_dir.glob("*.c")))
        commands.extend(
            _catalog_commands(
                entry,
                app_dir,
                build_root,
                kernel_tree=kernel_tree,
                arch=arch,
                toolchain_prefix=toolchain_prefix,
                kernel_version=kernel_version,
            )
        )
    return {
        "group": group,
        "entries": entries,
        "paths": [str(path) for path in sources],
        "commands": commands,
    }


def _rust_plan(
    group: str,
    linux_lab_root: Path,
    *,
    build_root: Path,
    kernel_tree: Path | None = None,
    arch: str = "",
    toolchain_prefix: str = "",
    kernel_version: str = "",
) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, group)
    rust_dirs = [_resolve_catalog_source(linux_lab_root, entry["source"]) for entry in entries]
    commands: list[list[str]] = []
    kernel_sample_sources: list[str] = []
    paths: list[str] = []
    for entry, rust_dir in zip(entries, rust_dirs, strict=True):
        paths.append(str(rust_dir))
        user_dir = rust_dir / "user"
        if user_dir.exists():
            paths.append(str(user_dir))
        commands.extend(
            _catalog_commands(
                entry,
                rust_dir,
                build_root,
                kernel_tree=kernel_tree,
                arch=arch,
                toolchain_prefix=toolchain_prefix,
                kernel_version=kernel_version,
            )
        )
        kernel_sample_sources.extend(str(path) for path in sorted(rust_dir.glob("*.rs")))
    return {
        "group": group,
        "entries": entries,
        "paths": paths,
        "commands": commands,
        "kernel_sample_sources": kernel_sample_sources,
    }


def _bpf_plan(
    group: str,
    linux_lab_root: Path,
    *,
    build_root: Path,
    kernel_tree: Path | None = None,
    arch: str = "",
    toolchain_prefix: str = "",
    kernel_version: str = "",
) -> dict[str, Any]:
    entries = _enabled_entries_for_group(linux_lab_root, group)
    sources: list[Path] = []
    commands: list[list[str]] = []
    for entry in entries:
        bpf_dir = _resolve_catalog_source(linux_lab_root, entry["source"])
        sources.extend(sorted(bpf_dir.glob("*.c")))
        commands.extend(
            _catalog_commands(
                entry,
                bpf_dir,
                build_root,
                kernel_tree=kernel_tree,
                arch=arch,
                toolchain_prefix=toolchain_prefix,
                kernel_version=kernel_version,
            )
        )
    return {
        "group": group,
        "entries": entries,
        "paths": [str(path) for path in sources],
        "commands": commands,
    }


def resolve_example_plan(
    *,
    example_groups: list[str],
    linux_lab_root: Path,
    build_root: Path,
    kernel_tree: Path | None = None,
    arch: str = "",
    toolchain_prefix: str = "",
    kernel_version: str = "",
) -> list[dict[str, Any]]:
    planners = {
        "modules-core": lambda: _modules_plan(
            "modules-core",
            linux_lab_root,
            build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
        "userspace-core": lambda: _userspace_plan(
            "userspace-core",
            linux_lab_root,
            build_root=build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
        "rust-core": lambda: _rust_plan(
            "rust-core",
            linux_lab_root,
            build_root=build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
        "bpf-core": lambda: _bpf_plan(
            "bpf-core",
            linux_lab_root,
            build_root=build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
        "modules-all": lambda: _modules_plan(
            "modules-all",
            linux_lab_root,
            build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
        "userspace-all": lambda: _userspace_plan(
            "userspace-all",
            linux_lab_root,
            build_root=build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
        "rust-all": lambda: _rust_plan(
            "rust-all",
            linux_lab_root,
            build_root=build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
        "bpf-all": lambda: _bpf_plan(
            "bpf-all",
            linux_lab_root,
            build_root=build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
    }

    resolved: list[dict[str, Any]] = []
    for group in EXAMPLE_GROUP_ORDER:
        if group in example_groups:
            resolved.append(planners[group]())
    return resolved
