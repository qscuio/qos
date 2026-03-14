from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
BIN_LINUX_LAB = ROOT / "linux-lab" / "bin" / "linux-lab"
REQUESTS_ROOT = ROOT / "build" / "linux-lab" / "requests"


def _read_yaml(path: Path) -> dict:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    assert isinstance(data, dict), f"{path} must decode to a mapping"
    return data


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _latest_request_root() -> Path:
    request_dirs = sorted([path for path in REQUESTS_ROOT.iterdir() if path.is_dir()])
    assert request_dirs, f"no request directories in {REQUESTS_ROOT}"
    return request_dirs[-1]


def setup_function() -> None:
    shutil.rmtree(ROOT / "build" / "linux-lab", ignore_errors=True)


def test_kernel_patch_matrix_is_ported() -> None:
    versions = ["4.19.317", "6.4.3", "6.9.6", "6.9.8", "6.10"]
    missing = [version for version in versions if not (ROOT / "linux-lab" / "patches" / f"linux-{version}" / "kernel.patch").is_file()]
    assert missing == [], f"missing linux-lab patch files: {missing}"


def test_kernel_baseline_configs_are_ported() -> None:
    x86_defconfig = ROOT / "linux-lab" / "configs" / "x86_64" / "defconfig"
    arm64_defconfig = ROOT / "linux-lab" / "configs" / "arm64" / "defconfig"

    assert x86_defconfig.is_file(), f"missing {x86_defconfig}"
    assert arm64_defconfig.is_file(), f"missing {arm64_defconfig}"

    assert "CONFIG_BPF_SYSCALL=y" in x86_defconfig.read_text(encoding="utf-8")
    assert "CONFIG_BPF_SYSCALL" in arm64_defconfig.read_text(encoding="utf-8")


def test_phase2_fragments_capture_qulk_feature_toggles() -> None:
    fragments = {
        "debug.conf": [
            "CONFIG_KGDB=y",
            "CONFIG_IKHEADERS=y",
            "CONFIG_KCOV=y",
            "CONFIG_PAGE_OWNER=y",
            "CONFIG_FUNCTION_TRACER=y",
            "CONFIG_FTRACE_SYSCALLS=y",
            "CONFIG_LOCK_STAT=y",
            "# CONFIG_RANDOMIZE_BASE is not set",
            "# CONFIG_DRM_I915 is not set",
        ],
        "bpf.conf": [
            "CONFIG_DEBUG_INFO_BTF=y",
            "CONFIG_BPF_SYSCALL=y",
            "CONFIG_BPF_JIT=y",
            "CONFIG_BPF_EVENTS=y",
            "CONFIG_USER_NS=y",
            "CONFIG_CGROUP_BPF=y",
            "CONFIG_RTC_DRV_PL031=y",
        ],
        "rust.conf": [
            "CONFIG_RUST=y",
            "CONFIG_SAMPLES_RUST=y",
            "CONFIG_SAMPLE_RUST_MINIMAL=m",
            "CONFIG_SAMPLE_RUST_PRINT=m",
            "CONFIG_SAMPLE_RUST_MISC_DEVICE=m",
            "CONFIG_SAMPLE_RUST_DRIVER_PLATFORM=m",
            "CONFIG_SAMPLE_RUST_DRIVER_FAUX=m",
            "CONFIG_SAMPLE_RUST_HELLO=m",
            "CONFIG_SAMPLE_RUST_CHARDEV=m",
        ],
    }

    for name, markers in fragments.items():
        path = ROOT / "linux-lab" / "fragments" / name
        assert path.is_file(), f"missing fragment {path}"
        text = path.read_text(encoding="utf-8")
        for marker in markers:
            assert marker in text, f"missing {marker!r} in {path}"


def test_manifests_reference_real_phase2_assets() -> None:
    kernel_manifest = _read_yaml(ROOT / "linux-lab" / "manifests" / "kernels" / "6.9.8.yaml")
    profile_manifest = _read_yaml(ROOT / "linux-lab" / "manifests" / "profiles" / "debug.yaml")

    assert kernel_manifest["patch_set"] == "linux-lab/patches/linux-6.9.8/kernel.patch"
    assert kernel_manifest["config_family"] == "modern"
    assert len(kernel_manifest["source_sha256"]) == 64
    assert profile_manifest["fragment_ref"] == "linux-lab/fragments/debug.conf"
    assert "debug-core" in profile_manifest["tool_groups"]


def test_plan_emits_phase2_kernel_asset_metadata() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "plan",
            "--kernel",
            "6.9.8",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "debug",
            "--profile",
            "bpf",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    fetch_state = json.loads((request_root / "state" / "fetch.json").read_text(encoding="utf-8"))
    patch_state = json.loads((request_root / "state" / "patch.json").read_text(encoding="utf-8"))
    config_state = json.loads((request_root / "state" / "configure.json").read_text(encoding="utf-8"))

    assert fetch_state["metadata"]["source_url"].endswith("linux-6.9.8.tar.xz")
    assert len(fetch_state["metadata"]["source_sha256"]) == 64
    assert patch_state["metadata"]["patch_path"] == "linux-lab/patches/linux-6.9.8/kernel.patch"
    assert config_state["metadata"]["base_config"] == "linux-lab/configs/x86_64/defconfig"
    assert config_state["metadata"]["fragment_refs"] == [
        "linux-lab/fragments/debug.conf",
        "linux-lab/fragments/bpf.conf",
    ]
