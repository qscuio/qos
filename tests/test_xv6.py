import subprocess
import time
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
XV6_IMG = ROOT / "xv6" / "xv6.img"
XV6_FS_IMG = ROOT / "xv6" / "fs.img"


@pytest.fixture(scope="module", autouse=True)
def build_xv6():
    result = subprocess.run(
        ["bash", "scripts/xv6/build.sh"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=300,
    )
    assert result.returncode == 0, (
        f"xv6 build failed:\n{result.stdout.decode(errors='replace')}"
    )


def test_xv6_build():
    assert XV6_IMG.exists(), f"xv6.img not found at {XV6_IMG}"
    assert XV6_FS_IMG.exists(), f"fs.img not found at {XV6_FS_IMG}"


def test_xv6_boot_smoke():
    proc = subprocess.Popen(
        [
            "qemu-system-i386",
            "-nographic",
            "-monitor", "none",
            "-serial", "stdio",
            "-m", "256",
            "-drive", f"file={XV6_IMG},index=0,media=disk,format=raw",
            "-drive", f"file={XV6_FS_IMG},index=1,media=disk,format=raw",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    try:
        out, _ = proc.communicate(timeout=30)
    except subprocess.TimeoutExpired:
        # Expected success path: QEMU is still running at the shell prompt.
        proc.kill()
        out, _ = proc.communicate()

    text = out.decode(errors="replace")
    assert "$ " in text, (
        f"xv6 shell prompt ('$ ') not found in QEMU output.\n"
        f"Output:\n{text}"
    )
