import os
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
LOG = ROOT / "build" / "linux1" / "logs" / "boot.log"
DISK = ROOT / "build" / "linux1" / "images" / "linux1-disk.img"
KERNEL = ROOT / "build" / "linux1" / "kernel" / "vmlinuz-1.0.0"
MANIFEST = ROOT / "linux-1.0.0" / "manifests" / "linux1-sources.lock"


@pytest.fixture(scope="module", autouse=True)
def build_linux1():
    timeout_sec = int(os.environ.get("LINUX1_BUILD_TIMEOUT_SEC", "1800"))
    result = subprocess.run(
        ["make", "linux1"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert result.returncode == 0, result.stdout.decode(errors="replace")


def test_linux1_manifest_is_owned_by_linux_1_0_0():
    assert MANIFEST.exists(), f"missing manifest: {MANIFEST}"


def test_linux1_kernel_build():
    assert KERNEL.exists(), f"missing kernel image: {KERNEL}"


def test_linux1_disk_boot_lilo():
    assert DISK.exists(), f"missing boot disk: {DISK}"
    text = LOG.read_text(errors="replace") if LOG.exists() else ""
    assert "LILO" in text, f"LILO marker not found in {LOG}\n{text}"


def test_linux1_boot_to_init_shell():
    text = LOG.read_text(errors="replace") if LOG.exists() else ""
    ordered = ["LILO", "Linux version 1.0.0", "linux1-init: start", "linux1-sh# "]
    pos = -1
    for marker in ordered:
        nxt = text.find(marker, pos + 1)
        assert nxt >= 0, f"marker missing: {marker}\n{text}"
        pos = nxt
