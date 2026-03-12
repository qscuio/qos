from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_proc_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_proc.so"
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
            str(ROOT / "c-os/kernel/proc/proc.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_proc_create_lookup_remove_flow() -> None:
    lib = ctypes.CDLL(str(_build_proc_lib()))
    lib.qos_proc_reset.argtypes = []
    lib.qos_proc_reset.restype = None
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_remove.argtypes = [ctypes.c_uint32]
    lib.qos_proc_remove.restype = ctypes.c_int
    lib.qos_proc_parent.argtypes = [ctypes.c_uint32]
    lib.qos_proc_parent.restype = ctypes.c_int32
    lib.qos_proc_alive.argtypes = [ctypes.c_uint32]
    lib.qos_proc_alive.restype = ctypes.c_int
    lib.qos_proc_count.argtypes = []
    lib.qos_proc_count.restype = ctypes.c_uint32

    lib.qos_proc_reset()
    assert lib.qos_proc_create(1, 0) == 0
    assert lib.qos_proc_create(2, 1) == 0
    assert lib.qos_proc_create(3, 1) == 0
    assert lib.qos_proc_count() == 3
    assert lib.qos_proc_parent(2) == 1
    assert lib.qos_proc_alive(3) == 1
    assert lib.qos_proc_remove(3) == 0
    assert lib.qos_proc_alive(3) == 0
    assert lib.qos_proc_count() == 2


def test_c_proc_rejects_duplicates_and_invalid_pid() -> None:
    lib = ctypes.CDLL(str(_build_proc_lib()))
    lib.qos_proc_reset.argtypes = []
    lib.qos_proc_reset.restype = None
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_remove.argtypes = [ctypes.c_uint32]
    lib.qos_proc_remove.restype = ctypes.c_int

    lib.qos_proc_reset()
    assert lib.qos_proc_create(0, 0) != 0
    assert lib.qos_proc_create(7, 0) == 0
    assert lib.qos_proc_create(7, 0) != 0
    assert lib.qos_proc_remove(9) != 0
