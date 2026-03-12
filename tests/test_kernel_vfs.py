from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_vfs_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_vfs.so"
    out.parent.mkdir(parents=True, exist_ok=True)
    build = _run(
        [
            "gcc",
            "-shared",
            "-fPIC",
            "-std=c11",
            "-O2",
            "-I",
            str(ROOT / "c-os"),
            "-o",
            str(out),
            str(ROOT / "c-os/kernel/init_state.c"),
            str(ROOT / "c-os/kernel/fs/vfs.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_vfs_create_exists_remove_flow() -> None:
    lib = ctypes.CDLL(str(_build_vfs_lib()))
    lib.qos_vfs_reset.argtypes = []
    lib.qos_vfs_reset.restype = None
    lib.qos_vfs_create.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_create.restype = ctypes.c_int
    lib.qos_vfs_exists.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_exists.restype = ctypes.c_int
    lib.qos_vfs_remove.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_remove.restype = ctypes.c_int
    lib.qos_vfs_count.argtypes = []
    lib.qos_vfs_count.restype = ctypes.c_uint32

    lib.qos_vfs_reset()
    assert lib.qos_vfs_create(b"/init") == 0
    assert lib.qos_vfs_create(b"/etc/config") == 0
    assert lib.qos_vfs_exists(b"/init") == 1
    assert lib.qos_vfs_exists(b"/etc/config") == 1
    assert lib.qos_vfs_count() == 2
    assert lib.qos_vfs_remove(b"/init") == 0
    assert lib.qos_vfs_exists(b"/init") == 0
    assert lib.qos_vfs_count() == 1


def test_c_vfs_rejects_duplicate_and_invalid_paths() -> None:
    lib = ctypes.CDLL(str(_build_vfs_lib()))
    lib.qos_vfs_reset.argtypes = []
    lib.qos_vfs_reset.restype = None
    lib.qos_vfs_create.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_create.restype = ctypes.c_int
    lib.qos_vfs_remove.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_remove.restype = ctypes.c_int

    lib.qos_vfs_reset()
    assert lib.qos_vfs_create(b"/tmp") == 0
    assert lib.qos_vfs_create(b"/tmp") != 0
    assert lib.qos_vfs_create(b"tmp") != 0
    assert lib.qos_vfs_create(b"") != 0
    assert lib.qos_vfs_remove(b"/missing") != 0
