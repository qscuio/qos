from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_workqueue_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_workqueue.so"
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
            str(ROOT / "c-os/kernel/workqueue/workqueue.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_workqueue_enqueue_cancel_and_drain() -> None:
    lib = ctypes.CDLL(str(_build_workqueue_lib()))
    cb_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    lib.qos_softirq_reset.argtypes = []
    lib.qos_softirq_reset.restype = None
    lib.qos_softirq_pending_mask.argtypes = []
    lib.qos_softirq_pending_mask.restype = ctypes.c_uint32
    lib.qos_softirq_run.argtypes = []
    lib.qos_softirq_run.restype = ctypes.c_uint32

    lib.qos_workqueue_reset.argtypes = []
    lib.qos_workqueue_reset.restype = None
    lib.qos_workqueue_enqueue.argtypes = [cb_type, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_workqueue_enqueue.restype = ctypes.c_int
    lib.qos_workqueue_cancel.argtypes = [ctypes.c_uint32]
    lib.qos_workqueue_cancel.restype = ctypes.c_int
    lib.qos_workqueue_pending_count.argtypes = []
    lib.qos_workqueue_pending_count.restype = ctypes.c_uint32
    lib.qos_workqueue_completed_count.argtypes = []
    lib.qos_workqueue_completed_count.restype = ctypes.c_uint32
    lib.workqueue_init.argtypes = []
    lib.workqueue_init.restype = None

    hits = ctypes.c_uint32(0)

    @cb_type
    def job(arg: ctypes.c_void_p) -> None:
        hits.value += ctypes.cast(arg, ctypes.c_void_p).value or 0

    w1 = ctypes.c_uint32(0)
    w2 = ctypes.c_uint32(0)

    lib.qos_softirq_reset()
    lib.qos_workqueue_reset()
    lib.workqueue_init()

    assert lib.qos_workqueue_enqueue(job, ctypes.c_void_p(1), ctypes.byref(w1)) == 0
    assert lib.qos_workqueue_enqueue(job, ctypes.c_void_p(2), ctypes.byref(w2)) == 0
    assert w1.value != 0
    assert w2.value != 0
    assert w1.value != w2.value

    assert lib.qos_workqueue_pending_count() == 2
    assert lib.qos_workqueue_completed_count() == 0
    assert (lib.qos_softirq_pending_mask() & (1 << 2)) != 0

    assert lib.qos_workqueue_cancel(w1.value) == 0
    assert lib.qos_workqueue_pending_count() == 1

    assert lib.qos_softirq_run() >= 1
    assert hits.value == 2
    assert lib.qos_workqueue_pending_count() == 0
    assert lib.qos_workqueue_completed_count() == 1
    assert lib.qos_workqueue_cancel(w1.value) == -1
