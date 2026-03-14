from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml


REQUIRED_FIELDS = {
    "key",
    "kind",
    "category",
    "source",
    "origin",
    "build_mode",
    "enabled",
}
ALLOWED_KINDS = {"module", "userspace", "rust", "bpf"}
ALLOWED_BUILD_MODES = {
    "kbuild-module",
    "gcc-userspace",
    "rust-user",
    "bpf-clang",
    "kernel-tree-rust",
    "custom-make",
    "disabled",
}


def _read_yaml(path: Path) -> dict[str, Any]:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a YAML mapping")
    return data


def _validate_build_commands(path: Path, item: dict[str, Any]) -> None:
    build_commands = item.get("build_commands")
    if build_commands is None:
        return
    if not isinstance(build_commands, list):
        raise ValueError(f"{path} build_commands must be a list")
    for index, command in enumerate(build_commands):
        if not isinstance(command, list) or not command or not all(isinstance(part, str) for part in command):
            raise ValueError(f"{path} build_commands[{index}] must be a non-empty list of strings")


def load_example_catalog(root: Path) -> dict[str, dict[str, Any]]:
    catalog: dict[str, dict[str, Any]] = {}
    for path in sorted(root.glob("*.yaml")):
        item = _read_yaml(path)
        missing = REQUIRED_FIELDS.difference(item)
        if missing:
            raise ValueError(f"{path} missing required fields: {sorted(missing)}")
        if item["kind"] not in ALLOWED_KINDS:
            raise ValueError(f"{path} has unsupported kind: {item['kind']!r}")
        if item["build_mode"] not in ALLOWED_BUILD_MODES:
            raise ValueError(f"{path} has unsupported build_mode: {item['build_mode']!r}")
        _validate_build_commands(path, item)
        key = item["key"]
        if key in catalog:
            raise ValueError(f"duplicate example catalog key: {key}")
        catalog[key] = item
    return catalog
