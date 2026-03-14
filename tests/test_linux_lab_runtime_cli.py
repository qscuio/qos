from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN_LINUX_LAB = ROOT / "linux-lab" / "bin" / "linux-lab"
REQUESTS_ROOT = ROOT / "build" / "linux-lab" / "requests"


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _latest_request_root() -> Path:
    request_dirs = sorted([path for path in REQUESTS_ROOT.iterdir() if path.is_dir()])
    assert request_dirs, f"no request directories in {REQUESTS_ROOT}"
    return request_dirs[-1]


def setup_function() -> None:
    shutil.rmtree(ROOT / "build" / "linux-lab", ignore_errors=True)


def test_run_dry_run_writes_runtime_stage_metadata() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel",
            "6.18.4",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "default-lab",
            "--dry-run",
            "--stop-after",
            "boot",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    boot_state = json.loads((request_root / "state" / "boot.json").read_text(encoding="utf-8"))
    image_state = json.loads((request_root / "state" / "build-image.json").read_text(encoding="utf-8"))
    kernel_state = json.loads((request_root / "state" / "build-kernel.json").read_text(encoding="utf-8"))

    assert boot_state["status"] == "dry-run"
    assert image_state["status"] == "dry-run"
    assert kernel_state["status"] == "dry-run"
    assert boot_state["metadata"]["qemu_command"][0] == "qemu-system-x86_64"
    assert image_state["metadata"]["image_path"].endswith("/images/noble.img")
    assert kernel_state["metadata"]["kernel_image_path"].endswith("/arch/x86_64/boot/bzImage")


def test_plan_remains_metadata_only_after_run_support() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "plan",
            "--kernel",
            "6.18.4",
            "--arch",
            "arm64",
            "--image",
            "jammy",
            "--profile",
            "debug",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    boot_state = json.loads((request_root / "state" / "boot.json").read_text(encoding="utf-8"))
    assert boot_state["status"] == "placeholder"

