from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

SYSCALL_OP_CONST = 1
SYSCALL_OP_ADD = 2


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_syscall_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_syscall.so"
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
            str(ROOT / "c-os/kernel/syscall/syscall.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_syscall_register_dispatch_unregister() -> None:
    lib = ctypes.CDLL(str(_build_syscall_lib()))
    lib.qos_syscall_reset.argtypes = []
    lib.qos_syscall_reset.restype = None
    lib.qos_syscall_register.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_int64]
    lib.qos_syscall_register.restype = ctypes.c_int
    lib.qos_syscall_unregister.argtypes = [ctypes.c_uint32]
    lib.qos_syscall_unregister.restype = ctypes.c_int
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64
    lib.qos_syscall_count.argtypes = []
    lib.qos_syscall_count.restype = ctypes.c_uint32

    lib.qos_syscall_reset()
    assert lib.qos_syscall_register(1, SYSCALL_OP_CONST, 42) == 0
    assert lib.qos_syscall_register(2, SYSCALL_OP_ADD, 0) == 0
    assert lib.qos_syscall_count() == 2
    assert lib.qos_syscall_dispatch(1, 0, 0, 0, 0) == 42
    assert lib.qos_syscall_dispatch(2, 7, 9, 0, 0) == 16
    assert lib.qos_syscall_unregister(1) == 0
    assert lib.qos_syscall_dispatch(1, 0, 0, 0, 0) == -1
    assert lib.qos_syscall_count() == 1


def test_c_syscall_rejects_invalid_inputs() -> None:
    lib = ctypes.CDLL(str(_build_syscall_lib()))
    lib.qos_syscall_reset.argtypes = []
    lib.qos_syscall_reset.restype = None
    lib.qos_syscall_register.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_int64]
    lib.qos_syscall_register.restype = ctypes.c_int

    lib.qos_syscall_reset()
    assert lib.qos_syscall_register(7, SYSCALL_OP_CONST, 1) == 0
    assert lib.qos_syscall_register(7, SYSCALL_OP_CONST, 2) != 0
    assert lib.qos_syscall_register(9999, SYSCALL_OP_CONST, 1) != 0
    assert lib.qos_syscall_register(8, 99, 0) != 0
