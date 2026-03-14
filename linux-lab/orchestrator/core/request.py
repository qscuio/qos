from __future__ import annotations

import hashlib
import json
import platform
from dataclasses import dataclass
from pathlib import Path

SCHEMA_VERSION = 1


class RequestValidationError(ValueError):
    """Raised when a Phase 1 request is invalid."""


@dataclass(frozen=True)
class ResolvedRequest:
    kernel_version: str
    arch: str
    image_release: str
    profiles: list[str]
    mirror_region: str | None
    compat_mode: bool
    legacy_args: dict[str, str]
    request_fingerprint: str
    artifact_root: Path
    command: str = "plan"
    dry_run: bool = False
    stop_after: str | None = None


def normalize_arch(value: str) -> str:
    mapping = {
        "x86_64": "x86_64",
        "amd64": "x86_64",
        "arm64": "arm64",
        "aarch64": "arm64",
    }
    normalized = mapping.get(value)
    if normalized is None:
        raise RequestValidationError(f"unsupported arch: {value}")
    return normalized


def _fingerprint(payload: dict[str, object]) -> str:
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()[:12]


def parse_ulk_args(raw_args: list[str]) -> dict[str, str]:
    allowed = {"arch", "kernel", "mirror"}
    parsed: dict[str, str] = {}
    for arg in raw_args:
        if arg.count("=") != 1:
            raise RequestValidationError(f"malformed compatibility argument: {arg}")
        key, value = arg.split("=", 1)
        if not key or not value:
            raise RequestValidationError(f"malformed compatibility argument: {arg}")
        if key not in allowed:
            raise RequestValidationError(f"unsupported compatibility key: {key}")
        if key in parsed:
            raise RequestValidationError(f"duplicate compatibility key: {key}")
        parsed[key] = value
    return parsed


def _expand_profiles(manifests, profile_keys: list[str]) -> list[str]:
    expanded: list[str] = []
    for key in profile_keys:
        if key not in manifests.profiles:
            raise RequestValidationError(f"unknown profile: {key}")
        profile = manifests.profiles[key]
        if profile.kind == "meta":
            expanded.extend(profile.expands_to)
        else:
            expanded.append(key)

    deduped: list[str] = []
    for key in expanded:
        if key not in deduped:
            deduped.append(key)

    removed: set[str] = set()
    for key in deduped:
        profile = manifests.profiles[key]
        removed.update(profile.replaces)

    concrete = [key for key in deduped if key not in removed]
    concrete_profiles = [manifests.profiles[key] for key in concrete]
    concrete_profiles.sort(key=lambda profile: (profile.precedence, profile.key))
    return [profile.key for profile in concrete_profiles]


def _validate_request(manifests, kernel_key: str, arch_key: str, image_key: str, profiles: list[str], mirror: str | None) -> None:
    if kernel_key not in manifests.kernels:
        raise RequestValidationError(f"unknown kernel: {kernel_key}")
    if arch_key not in manifests.arches:
        raise RequestValidationError(f"unknown arch: {arch_key}")
    if image_key not in manifests.images:
        raise RequestValidationError(f"unknown image: {image_key}")

    kernel = manifests.kernels[kernel_key]
    arch = manifests.arches[arch_key]
    image = manifests.images[image_key]

    if arch_key not in kernel.supported_arches:
        raise RequestValidationError(f"kernel {kernel_key} does not support arch {arch_key}")
    if image_key not in arch.supported_images:
        raise RequestValidationError(f"arch {arch_key} does not support image {image_key}")
    if mirror is not None and mirror not in image.mirror_regions:
        raise RequestValidationError(f"unsupported mirror {mirror!r} for image {image_key}")

    missing_required = [profile for profile in kernel.required_profiles if profile not in profiles]
    if missing_required:
        raise RequestValidationError(
            f"kernel {kernel_key} requires profiles {missing_required}, missing from request"
        )

    for profile_key in profiles:
        if profile_key not in kernel.supported_profiles:
            raise RequestValidationError(f"kernel {kernel_key} does not support profile {profile_key}")
        profile = manifests.profiles[profile_key]
        if profile.compatible_kernels != ["*"] and kernel_key not in profile.compatible_kernels:
            raise RequestValidationError(
                f"profile {profile_key} is not compatible with kernel {kernel_key}"
            )
        if profile.compatible_arches != ["*"] and arch_key not in profile.compatible_arches:
            raise RequestValidationError(f"profile {profile_key} is not compatible with arch {arch_key}")


def _resolve_request(
    manifests,
    *,
    kernel: str,
    arch: str,
    image: str,
    profiles: list[str],
    mirror: str | None,
    compat_mode: bool,
    legacy_args: dict[str, str],
    command: str,
    dry_run: bool = False,
    stop_after: str | None = None,
) -> ResolvedRequest:
    arch_key = normalize_arch(arch)
    concrete_profiles = _expand_profiles(manifests, profiles)
    _validate_request(manifests, kernel, arch_key, image, concrete_profiles, mirror)

    fingerprint = _fingerprint(
        {
            "kernel_version": kernel,
            "arch": arch_key,
            "image_release": image,
            "profiles": concrete_profiles,
            "mirror_region": mirror,
            "compat_mode": compat_mode,
            "command": command,
            "dry_run": dry_run,
            "stop_after": stop_after,
            "schema_version": SCHEMA_VERSION,
        }
    )
    artifact_root = Path("build") / "linux-lab" / "requests" / fingerprint
    return ResolvedRequest(
        kernel_version=kernel,
        arch=arch_key,
        image_release=image,
        profiles=concrete_profiles,
        mirror_region=mirror,
        compat_mode=compat_mode,
        legacy_args=legacy_args,
        request_fingerprint=fingerprint,
        artifact_root=artifact_root,
        command=command,
        dry_run=dry_run,
        stop_after=stop_after,
    )


def resolve_native_request(
    *,
    manifests,
    kernel: str | None,
    arch: str | None,
    image: str | None,
    profiles: list[str] | None,
    mirror: str | None,
    command: str = "plan",
    dry_run: bool = False,
    stop_after: str | None = None,
) -> ResolvedRequest:
    if not kernel:
        raise RequestValidationError("--kernel is required")
    if not arch:
        raise RequestValidationError("--arch is required")
    if not image:
        raise RequestValidationError("--image is required")
    if not profiles:
        raise RequestValidationError("at least one --profile is required")

    return _resolve_request(
        manifests,
        kernel=kernel,
        arch=arch,
        image=image,
        profiles=profiles,
        mirror=mirror,
        compat_mode=False,
        legacy_args={},
        command=command,
        dry_run=dry_run,
        stop_after=stop_after,
    )


def resolve_compat_request(*, manifests, raw_args: list[str]) -> ResolvedRequest:
    legacy_args = parse_ulk_args(raw_args)
    arch = legacy_args.get("arch")
    if arch is None:
        arch = normalize_arch(platform.machine())

    return _resolve_request(
        manifests,
        kernel=legacy_args.get("kernel", manifests.default_kernel),
        arch=arch,
        image=manifests.default_image,
        profiles=["default-lab"],
        mirror=legacy_args.get("mirror"),
        compat_mode=True,
        legacy_args=legacy_args,
        command="plan",
    )
