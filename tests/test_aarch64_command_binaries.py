from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
CMDS = ("echo", "ps", "ping", "ip", "wget", "ls", "cat", "touch", "edit")


@pytest.mark.parametrize("impl", ["c", "rust"])
@pytest.mark.parametrize("arch", ["aarch64", "x86_64"])
def test_build_emits_per_command_elf_binaries(impl: str, arch: str) -> None:
    build = subprocess.run(
        ["make", "-C", f"{impl}-os", f"ARCH={arch}", "build"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    assert build.returncode == 0, f"stderr:\n{build.stderr}\nstdout:\n{build.stdout}"

    bin_dir = ROOT / f"{impl}-os" / "build" / arch / "rootfs" / "bin"
    assert bin_dir.exists(), f"missing bin dir: {bin_dir}"

    for cmd in CMDS:
        path = bin_dir / cmd
        assert path.exists(), f"missing command binary: {path}"
        assert path.is_file(), f"not a file: {path}"
        data = path.read_bytes()
        assert len(data) >= 4, f"binary too small: {path}"
        assert data[:4] == b"\x7fELF", f"not an ELF binary: {path}"
