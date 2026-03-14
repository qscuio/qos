from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml


VALID_BUILD_POLICIES = {"host-build", "guest-build", "checkout-only"}
TOOL_GROUP_MAP = {
    "debug-tools": ["crash", "cgdb"],
    "bpf-core": ["libbpf-bootstrap", "retsnoop"],
}

TOOL_KEY_ORDER = ["crash", "cgdb", "libbpf-bootstrap", "retsnoop"]


def _read_yaml(path: Path) -> dict[str, Any]:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a YAML mapping")
    return data


def _normalize_command_list(raw: Any, *, field: str, path: Path) -> list[list[str]]:
    if raw is None:
        return []
    if not isinstance(raw, list):
        raise ValueError(f"{path} field {field} must be a list")
    commands: list[list[str]] = []
    for index, command in enumerate(raw):
        if not isinstance(command, list) or not all(isinstance(part, str) for part in command):
            raise ValueError(f"{path} field {field}[{index}] must be a list of strings")
        commands.append(command)
    return commands


def _normalize_asset_copies(raw: Any, *, path: Path) -> list[dict[str, str]]:
    if raw is None:
        return []
    if not isinstance(raw, list):
        raise ValueError(f"{path} field post_prepare_asset_copies must be a list")
    copies: list[dict[str, str]] = []
    for index, item in enumerate(raw):
        if not isinstance(item, dict):
            raise ValueError(f"{path} field post_prepare_asset_copies[{index}] must be a mapping")
        source = item.get("from")
        destination = item.get("to")
        if not isinstance(source, str) or not isinstance(destination, str):
            raise ValueError(f"{path} field post_prepare_asset_copies[{index}] must include string from/to")
        source_path = Path(source)
        destination_path = Path(destination)
        if source_path.is_absolute() or ".." in source_path.parts:
            raise ValueError(f"{path} field post_prepare_asset_copies[{index}].from must be repo-relative")
        if destination_path.is_absolute() or ".." in destination_path.parts:
            raise ValueError(f"{path} field post_prepare_asset_copies[{index}].to must be checkout-relative")
        copies.append({"from": source, "to": destination})
    return copies


def _resolve_repo_relative_path(raw_path: str, *, linux_lab_root: Path) -> Path:
    path = Path(raw_path)
    if linux_lab_root.name != "linux-lab" and path.parts[:1] == ("linux-lab",):
        return linux_lab_root / Path(*path.parts[1:])
    repo_root = linux_lab_root.parent if linux_lab_root.name == "linux-lab" else linux_lab_root
    return repo_root / path


def load_tool_manifests(root: Path) -> dict[str, dict[str, Any]]:
    manifests: dict[str, dict[str, Any]] = {}
    for path in sorted(root.glob("*.yaml")):
        manifest = _read_yaml(path)
        build_policy = manifest.get("build_policy", "host-build")
        if build_policy not in VALID_BUILD_POLICIES:
            raise ValueError(f"{path} has unsupported build_policy: {build_policy}")
        normalized = dict(manifest)
        normalized["build_policy"] = build_policy
        normalized["prepare_commands"] = _normalize_command_list(manifest.get("prepare_commands"), field="prepare_commands", path=path)
        normalized["build_commands"] = _normalize_command_list(manifest.get("build_commands"), field="build_commands", path=path)
        normalized["guest_build_commands"] = _normalize_command_list(
            manifest.get("guest_build_commands"),
            field="guest_build_commands",
            path=path,
        )
        normalized["post_prepare_asset_copies"] = _normalize_asset_copies(
            manifest.get("post_prepare_asset_copies"),
            path=path,
        )
        manifests[normalized["key"]] = normalized
    return manifests


def resolve_tool_keys(tool_groups: list[str]) -> list[str]:
    resolved: list[str] = []
    for group in tool_groups:
        for key in TOOL_GROUP_MAP.get(group, []):
            if key not in resolved:
                resolved.append(key)
    return sorted(resolved, key=lambda key: TOOL_KEY_ORDER.index(key) if key in TOOL_KEY_ORDER else len(TOOL_KEY_ORDER))


def resolve_tool_plan(*, tool_keys: list[str], linux_lab_root: Path) -> list[dict[str, Any]]:
    manifests = load_tool_manifests(linux_lab_root / "tools")
    plan: list[dict[str, Any]] = []
    workspace_root = linux_lab_root.parent
    for key in tool_keys:
        manifest = manifests[key]
        checkout_dir = Path(manifest["checkout_dir"])
        if not checkout_dir.is_absolute():
            checkout_dir = workspace_root / checkout_dir
        build_workdir = checkout_dir
        raw_build_workdir = manifest.get("build_workdir")
        if raw_build_workdir:
            build_workdir = Path(raw_build_workdir)
            if not build_workdir.is_absolute():
                build_workdir = checkout_dir / build_workdir
        asset_copies: list[dict[str, str]] = []
        for copy_rule in manifest["post_prepare_asset_copies"]:
            source = _resolve_repo_relative_path(copy_rule["from"], linux_lab_root=linux_lab_root)
            asset_copies.append({"from": str(source), "to": copy_rule["to"]})
        plan.append(
            {
                "key": key,
                "repo_url": manifest["repo_url"],
                "build_policy": manifest["build_policy"],
                "checkout_dir": str(checkout_dir),
                "build_workdir": str(build_workdir),
                "clone_command": ["git", "clone", manifest["repo_url"], str(checkout_dir)],
                "prepare_commands": manifest["prepare_commands"],
                "build_commands": manifest["build_commands"],
                "guest_build_commands": manifest["guest_build_commands"],
                "post_prepare_asset_copies": asset_copies,
            }
        )
    return plan


def resolve_kernel_tool_plan(*, request, manifests) -> list[dict[str, Any]]:
    arch = manifests.arches[request.arch]
    workspace_root = Path(request.artifact_root).resolve() / "workspace"
    build_root = workspace_root / "build"
    kernel_tree = workspace_root / "kernel" / f"linux-{request.kernel_version}"
    return [
        {
            "name": "tools/libapi",
            "command": [
                "make",
                f"O={build_root}",
                f"ARCH={request.arch}",
                f"CROSS_COMPILE={arch.toolchain_prefix}",
                "tools/libapi",
            ],
        },
        {
            "name": "kernel-tools",
            "command": [
                "make",
                "-C",
                str(kernel_tree / "tools"),
                f"O={build_root}",
                "subdir=tools",
                "all",
            ],
        },
    ]
