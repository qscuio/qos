from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_kthread_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_kthread.so"
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
            str(ROOT / "c-os/kernel/kthread/kthread.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_kthread_create_run_stop_wake() -> None:
    lib = ctypes.CDLL(str(_build_kthread_lib()))
    cb_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    lib.qos_kthread_reset.argtypes = []
    lib.qos_kthread_reset.restype = None
    lib.qos_kthread_create.argtypes = [cb_type, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_kthread_create.restype = ctypes.c_int
    lib.qos_kthread_wake.argtypes = [ctypes.c_uint32]
    lib.qos_kthread_wake.restype = ctypes.c_int
    lib.qos_kthread_stop.argtypes = [ctypes.c_uint32]
    lib.qos_kthread_stop.restype = ctypes.c_int
    lib.qos_kthread_count.argtypes = []
    lib.qos_kthread_count.restype = ctypes.c_uint32
    lib.qos_kthread_run_count.argtypes = [ctypes.c_uint32]
    lib.qos_kthread_run_count.restype = ctypes.c_uint32
    lib.qos_kthread_run_next.argtypes = []
    lib.qos_kthread_run_next.restype = ctypes.c_uint32

    t1_hits = ctypes.c_uint32(0)
    t2_hits = ctypes.c_uint32(0)

    @cb_type
    def thread_main(arg: ctypes.c_void_p) -> None:
        ptr = ctypes.cast(arg, ctypes.POINTER(ctypes.c_uint32))
        ptr[0] += 1

    tid1 = ctypes.c_uint32(0)
    tid2 = ctypes.c_uint32(0)

    lib.qos_kthread_reset()
    assert lib.qos_kthread_create(thread_main, ctypes.cast(ctypes.pointer(t1_hits), ctypes.c_void_p), ctypes.byref(tid1)) == 0
    assert lib.qos_kthread_create(thread_main, ctypes.cast(ctypes.pointer(t2_hits), ctypes.c_void_p), ctypes.byref(tid2)) == 0
    assert tid1.value != 0
    assert tid2.value != 0
    assert tid1.value != tid2.value
    assert lib.qos_kthread_count() == 2

    assert lib.qos_kthread_run_next() == tid1.value
    assert lib.qos_kthread_run_next() == tid2.value
    assert t1_hits.value == 1
    assert t2_hits.value == 1

    assert lib.qos_kthread_stop(tid1.value) == 0
    assert lib.qos_kthread_run_next() == tid2.value
    assert t2_hits.value == 2

    assert lib.qos_kthread_wake(tid1.value) == 0
    assert lib.qos_kthread_run_next() == tid1.value
    assert t1_hits.value == 2
    assert lib.qos_kthread_run_count(tid1.value) == 2
    assert lib.qos_kthread_run_count(tid2.value) == 2


def test_c_kthread_rejects_invalid_tid_controls() -> None:
    lib = ctypes.CDLL(str(_build_kthread_lib()))

    lib.qos_kthread_reset.argtypes = []
    lib.qos_kthread_reset.restype = None
    lib.qos_kthread_wake.argtypes = [ctypes.c_uint32]
    lib.qos_kthread_wake.restype = ctypes.c_int
    lib.qos_kthread_stop.argtypes = [ctypes.c_uint32]
    lib.qos_kthread_stop.restype = ctypes.c_int

    lib.qos_kthread_reset()
    assert lib.qos_kthread_wake(12345) == -1
    assert lib.qos_kthread_stop(12345) == -1
