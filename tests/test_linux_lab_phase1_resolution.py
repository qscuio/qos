from __future__ import annotations

import importlib.util
import shutil
import subprocess
import sys
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
LINUX_LAB_ROOT = ROOT / "linux-lab"
BIN_LINUX_LAB = LINUX_LAB_ROOT / "bin" / "linux-lab"
BIN_ULK = LINUX_LAB_ROOT / "bin" / "ulk"
MANIFESTS_MODULE = LINUX_LAB_ROOT / "orchestrator" / "core" / "manifests.py"
REQUEST_MODULE = LINUX_LAB_ROOT / "orchestrator" / "core" / "request.py"


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def setup_function() -> None:
    shutil.rmtree(ROOT / "build" / "linux-lab", ignore_errors=True)


def test_manifest_defaults_and_schema_contract() -> None:
    manifests_mod = _load_module("linux_lab_manifests", MANIFESTS_MODULE)
    request_mod = _load_module("linux_lab_request", REQUEST_MODULE)

    manifests = manifests_mod.load_manifests(LINUX_LAB_ROOT / "manifests")
    request = request_mod.resolve_native_request(
        manifests=manifests,
        kernel="6.18.4",
        arch="x86_64",
        image="noble",
        profiles=["default-lab"],
        mirror=None,
    )

    assert manifests.default_kernel == "6.18.4"
    assert manifests.default_image == "noble"
    assert request.kernel_version == "6.18.4"
    assert request.image_release == "noble"
    assert request.arch == "x86_64"
    assert request.profiles == ["debug", "bpf", "rust", "samples", "debug-tools"]
    assert request.artifact_root.name == request.request_fingerprint


def test_full_samples_meta_profile_expands_to_broad_concrete_profiles() -> None:
    manifests_mod = _load_module("linux_lab_manifests_full_samples", MANIFESTS_MODULE)
    request_mod = _load_module("linux_lab_request_full_samples", REQUEST_MODULE)

    manifests = manifests_mod.load_manifests(LINUX_LAB_ROOT / "manifests")
    request = request_mod.resolve_native_request(
        manifests=manifests,
        kernel="6.18.4",
        arch="x86_64",
        image="noble",
        profiles=["full-samples"],
        mirror=None,
    )

    assert request.profiles == ["bpf-all", "rust-all", "samples-all"]


def test_validate_is_read_only() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "validate",
            "--kernel",
            "6.18.4",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "default-lab",
        ]
    )
    assert result.returncode == 0, result.stderr
    requests_root = ROOT / "build" / "linux-lab" / "requests"
    assert not requests_root.exists(), "validate must be read-only in phase 1"


def test_ulk_parser_rejects_duplicate_keys() -> None:
    result = _run([str(BIN_ULK), "arch=x86_64", "arch=arm64"])
    assert result.returncode != 0
    assert "duplicate" in result.stderr.lower()


def test_invalid_mirror_is_rejected() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "validate",
            "--kernel",
            "6.18.4",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "default-lab",
            "--mirror",
            "invalid",
        ]
    )
    assert result.returncode != 0
    assert "mirror" in result.stderr.lower()
