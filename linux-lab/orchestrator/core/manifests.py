from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml


class ManifestValidationError(ValueError):
    """Raised when manifest files are missing required Phase 1 fields."""


def _ensure_list(value: Any, field_name: str) -> list[str]:
    if value is None:
        return []
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise ManifestValidationError(f"{field_name} must be a list of strings")
    return value


def _ensure_bool(value: Any, field_name: str) -> bool:
    if not isinstance(value, bool):
        raise ManifestValidationError(f"{field_name} must be a boolean")
    return value


def _ensure_str(value: Any, field_name: str, *, required: bool = True) -> str | None:
    if value is None and not required:
        return None
    if not isinstance(value, str):
        raise ManifestValidationError(f"{field_name} must be a string")
    return value


def _ensure_int(value: Any, field_name: str) -> int:
    if not isinstance(value, int):
        raise ManifestValidationError(f"{field_name} must be an integer")
    return value


@dataclass(frozen=True)
class KernelManifest:
    key: str
    supported_arches: list[str]
    supported_profiles: list[str]
    required_profiles: list[str]
    default_compat: bool
    source_url: str | None
    source_sha256: str | None
    patch_set: str | None
    config_family: str | None
    tool_groups: list[str]
    example_groups: list[str]


@dataclass(frozen=True)
class ArchManifest:
    key: str
    toolchain_prefix: str
    kernel_image_name: str
    qemu_machine: str
    qemu_cpu: str
    console: str
    supported_images: list[str]


@dataclass(frozen=True)
class ImageManifest:
    key: str
    default_compat: bool
    mirror_regions: list[str]
    recipe_ref: str | None


@dataclass(frozen=True)
class ProfileManifest:
    key: str
    kind: str
    precedence: int
    replaces: list[str]
    expands_to: list[str]
    fragment_ref: str | None
    compatible_kernels: list[str]
    compatible_arches: list[str]
    host_tools: list[str]
    tool_groups: list[str]
    example_groups: list[str]
    post_build_refs: list[str]


@dataclass(frozen=True)
class ManifestCollection:
    kernels: dict[str, KernelManifest]
    arches: dict[str, ArchManifest]
    images: dict[str, ImageManifest]
    profiles: dict[str, ProfileManifest]
    default_kernel: str
    default_image: str


def _read_yaml(path: Path) -> dict[str, Any]:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ManifestValidationError(f"{path} must contain a YAML mapping")
    return data


def _load_kernel(path: Path) -> KernelManifest:
    raw = _read_yaml(path)
    return KernelManifest(
        key=_ensure_str(raw.get("key"), f"{path}:key"),
        supported_arches=_ensure_list(raw.get("supported_arches"), f"{path}:supported_arches"),
        supported_profiles=_ensure_list(raw.get("supported_profiles"), f"{path}:supported_profiles"),
        required_profiles=_ensure_list(raw.get("required_profiles"), f"{path}:required_profiles"),
        default_compat=_ensure_bool(raw.get("default_compat"), f"{path}:default_compat"),
        source_url=_ensure_str(raw.get("source_url"), f"{path}:source_url", required=False),
        source_sha256=_ensure_str(raw.get("source_sha256"), f"{path}:source_sha256", required=False),
        patch_set=_ensure_str(raw.get("patch_set"), f"{path}:patch_set", required=False),
        config_family=_ensure_str(raw.get("config_family"), f"{path}:config_family", required=False),
        tool_groups=_ensure_list(raw.get("tool_groups"), f"{path}:tool_groups"),
        example_groups=_ensure_list(raw.get("example_groups"), f"{path}:example_groups"),
    )


def _load_arch(path: Path) -> ArchManifest:
    raw = _read_yaml(path)
    return ArchManifest(
        key=_ensure_str(raw.get("key"), f"{path}:key"),
        toolchain_prefix=_ensure_str(raw.get("toolchain_prefix"), f"{path}:toolchain_prefix"),
        kernel_image_name=_ensure_str(raw.get("kernel_image_name"), f"{path}:kernel_image_name"),
        qemu_machine=_ensure_str(raw.get("qemu_machine"), f"{path}:qemu_machine"),
        qemu_cpu=_ensure_str(raw.get("qemu_cpu"), f"{path}:qemu_cpu"),
        console=_ensure_str(raw.get("console"), f"{path}:console"),
        supported_images=_ensure_list(raw.get("supported_images"), f"{path}:supported_images"),
    )


def _load_image(path: Path) -> ImageManifest:
    raw = _read_yaml(path)
    return ImageManifest(
        key=_ensure_str(raw.get("key"), f"{path}:key"),
        default_compat=_ensure_bool(raw.get("default_compat"), f"{path}:default_compat"),
        mirror_regions=_ensure_list(raw.get("mirror_regions"), f"{path}:mirror_regions"),
        recipe_ref=_ensure_str(raw.get("recipe_ref"), f"{path}:recipe_ref", required=False),
    )


def _load_profile(path: Path) -> ProfileManifest:
    raw = _read_yaml(path)
    kind = _ensure_str(raw.get("kind"), f"{path}:kind")
    if kind not in {"concrete", "meta"}:
        raise ManifestValidationError(f"{path}:kind must be 'concrete' or 'meta'")
    expands_to = _ensure_list(raw.get("expands_to"), f"{path}:expands_to")
    if kind != "meta" and expands_to:
        raise ManifestValidationError(f"{path}:expands_to is only valid for meta profiles")
    return ProfileManifest(
        key=_ensure_str(raw.get("key"), f"{path}:key"),
        kind=kind,
        precedence=_ensure_int(raw.get("precedence"), f"{path}:precedence"),
        replaces=_ensure_list(raw.get("replaces"), f"{path}:replaces"),
        expands_to=expands_to,
        fragment_ref=_ensure_str(raw.get("fragment_ref"), f"{path}:fragment_ref", required=False),
        compatible_kernels=_ensure_list(raw.get("compatible_kernels", ["*"]), f"{path}:compatible_kernels"),
        compatible_arches=_ensure_list(raw.get("compatible_arches", ["*"]), f"{path}:compatible_arches"),
        host_tools=_ensure_list(raw.get("host_tools"), f"{path}:host_tools"),
        tool_groups=_ensure_list(raw.get("tool_groups"), f"{path}:tool_groups"),
        example_groups=_ensure_list(raw.get("example_groups"), f"{path}:example_groups"),
        post_build_refs=_ensure_list(raw.get("post_build_refs"), f"{path}:post_build_refs"),
    )


def _load_family[T](root: Path, loader) -> dict[str, T]:
    items: dict[str, T] = {}
    for path in sorted(root.glob("*.yaml")):
        item = loader(path)
        key = getattr(item, "key")
        if key in items:
            raise ManifestValidationError(f"duplicate manifest key {key!r} in {root}")
        items[key] = item
    if not items:
        raise ManifestValidationError(f"manifest family is empty: {root}")
    return items


def _select_default[T](items: dict[str, T], family_name: str) -> str:
    defaults = [key for key, item in items.items() if getattr(item, "default_compat", False)]
    if len(defaults) != 1:
        raise ManifestValidationError(
            f"{family_name} must have exactly one default_compat item, found {defaults!r}"
        )
    return defaults[0]


def load_manifests(manifest_root: Path) -> ManifestCollection:
    kernels = _load_family(manifest_root / "kernels", _load_kernel)
    arches = _load_family(manifest_root / "arches", _load_arch)
    images = _load_family(manifest_root / "images", _load_image)
    profiles = _load_family(manifest_root / "profiles", _load_profile)

    return ManifestCollection(
        kernels=kernels,
        arches=arches,
        images=images,
        profiles=profiles,
        default_kernel=_select_default(kernels, "kernels"),
        default_image=_select_default(images, "images"),
    )
