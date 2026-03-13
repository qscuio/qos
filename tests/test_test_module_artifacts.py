from __future__ import annotations

import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _read_elf_header(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    assert len(data) >= 64, f"{path} is too small to be ELF"
    assert data[:4] == b"\x7fELF", f"{path} missing ELF magic"
    e_type = struct.unpack_from("<H", data, 16)[0]
    e_machine = struct.unpack_from("<H", data, 18)[0]
    return e_type, e_machine


def test_build_test_module_artifacts_from_source() -> None:
    out_rootfs = ROOT / "c-os/build/test-modules-rootfs"
    out_rootfs.mkdir(parents=True, exist_ok=True)

    build = _run([str(ROOT / "scripts/build_test_modules.sh"), "x86_64", str(out_rootfs)])
    assert build.returncode == 0, f"stderr:\n{build.stderr}\nstdout:\n{build.stdout}"

    so_path = out_rootfs / "lib/libqos_test.so"
    ko_path = out_rootfs / "lib/modules/qos_test.ko"
    assert so_path.exists(), f"missing {so_path}"
    assert ko_path.exists(), f"missing {ko_path}"

    so_type, so_machine = _read_elf_header(so_path)
    ko_type, ko_machine = _read_elf_header(ko_path)

    assert so_type == 3
    assert ko_type == 3
    assert so_machine == 62
    assert ko_machine == 62
