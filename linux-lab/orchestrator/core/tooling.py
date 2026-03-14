from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml


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


def load_tool_manifests(root: Path) -> dict[str, dict[str, Any]]:
    manifests: dict[str, dict[str, Any]] = {}
    for path in sorted(root.glob("*.yaml")):
        manifest = _read_yaml(path)
        manifests[manifest["key"]] = manifest
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
    for key in tool_keys:
        manifest = manifests[key]
        checkout_dir = Path(manifest["checkout_dir"])
        plan.append(
            {
                "key": key,
                "repo_url": manifest["repo_url"],
                "checkout_dir": str(checkout_dir),
                "clone_command": ["git", "clone", manifest["repo_url"], str(checkout_dir)],
                "prepare_commands": manifest.get("prepare_commands", []),
                "build_commands": manifest.get("build_commands", []),
            }
        )
    return plan
