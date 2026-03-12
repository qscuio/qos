from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_softirq_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_softirq.so"
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
            str(ROOT / "c-os/kernel/irq/softirq.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_softirq_register_raise_and_run() -> None:
    lib = ctypes.CDLL(str(_build_softirq_lib()))
    cb_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    lib.qos_softirq_reset.argtypes = []
    lib.qos_softirq_reset.restype = None
    lib.qos_softirq_register.argtypes = [ctypes.c_uint32, cb_type, ctypes.c_void_p]
    lib.qos_softirq_register.restype = ctypes.c_int
    lib.qos_softirq_raise.argtypes = [ctypes.c_uint32]
    lib.qos_softirq_raise.restype = ctypes.c_int
    lib.qos_softirq_pending_mask.argtypes = []
    lib.qos_softirq_pending_mask.restype = ctypes.c_uint32
    lib.qos_softirq_run.argtypes = []
    lib.qos_softirq_run.restype = ctypes.c_uint32

    hits = ctypes.c_uint32(0)

    @cb_type
    def handler(ctx: ctypes.c_void_p) -> None:
        ptr = ctypes.cast(ctx, ctypes.POINTER(ctypes.c_uint32))
        ptr[0] += 1

    lib.qos_softirq_reset()
    assert lib.qos_softirq_register(2, handler, ctypes.cast(ctypes.pointer(hits), ctypes.c_void_p)) == 0
    assert lib.qos_softirq_pending_mask() == 0
    assert lib.qos_softirq_raise(2) == 0
    assert lib.qos_softirq_pending_mask() == (1 << 2)
    assert lib.qos_softirq_run() == 1
    assert hits.value == 1
    assert lib.qos_softirq_pending_mask() == 0


def test_c_softirq_rejects_invalid_vectors() -> None:
    lib = ctypes.CDLL(str(_build_softirq_lib()))
    cb_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    lib.qos_softirq_reset.argtypes = []
    lib.qos_softirq_reset.restype = None
    lib.qos_softirq_register.argtypes = [ctypes.c_uint32, cb_type, ctypes.c_void_p]
    lib.qos_softirq_register.restype = ctypes.c_int
    lib.qos_softirq_raise.argtypes = [ctypes.c_uint32]
    lib.qos_softirq_raise.restype = ctypes.c_int

    @cb_type
    def handler(_ctx: ctypes.c_void_p) -> None:
        return None

    lib.qos_softirq_reset()
    assert lib.qos_softirq_register(8, handler, None) == -1
    assert lib.qos_softirq_raise(8) == -1
