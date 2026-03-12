from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_timer_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_timer.so"
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
            str(ROOT / "c-os/kernel/timer/timer.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_timer_one_shot_and_periodic_tick_flow() -> None:
    lib = ctypes.CDLL(str(_build_timer_lib()))
    cb_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    lib.qos_softirq_reset.argtypes = []
    lib.qos_softirq_reset.restype = None
    lib.qos_softirq_pending_mask.argtypes = []
    lib.qos_softirq_pending_mask.restype = ctypes.c_uint32
    lib.qos_softirq_run.argtypes = []
    lib.qos_softirq_run.restype = ctypes.c_uint32

    lib.qos_timer_reset.argtypes = []
    lib.qos_timer_reset.restype = None
    lib.qos_timer_add.argtypes = [ctypes.c_uint32, ctypes.c_uint32, cb_type, ctypes.c_void_p]
    lib.qos_timer_add.restype = ctypes.c_int32
    lib.qos_timer_cancel.argtypes = [ctypes.c_int32]
    lib.qos_timer_cancel.restype = ctypes.c_int
    lib.qos_timer_tick.argtypes = [ctypes.c_uint32]
    lib.qos_timer_tick.restype = None
    lib.qos_timer_active_count.argtypes = []
    lib.qos_timer_active_count.restype = ctypes.c_uint32
    lib.qos_timer_pending_count.argtypes = []
    lib.qos_timer_pending_count.restype = ctypes.c_uint32
    lib.timer_init.argtypes = []
    lib.timer_init.restype = None

    hits = ctypes.c_uint32(0)

    @cb_type
    def callback(ctx: ctypes.c_void_p) -> None:
        ptr = ctypes.cast(ctx, ctypes.POINTER(ctypes.c_uint32))
        ptr[0] += 1

    lib.qos_softirq_reset()
    lib.qos_timer_reset()
    lib.timer_init()

    timer_id = lib.qos_timer_add(3, 0, callback, ctypes.cast(ctypes.pointer(hits), ctypes.c_void_p))
    assert timer_id > 0
    assert lib.qos_timer_active_count() == 1

    lib.qos_timer_tick(2)
    assert lib.qos_timer_pending_count() == 0
    assert hits.value == 0

    lib.qos_timer_tick(1)
    assert lib.qos_timer_pending_count() == 1
    assert (lib.qos_softirq_pending_mask() & 1) != 0
    assert lib.qos_softirq_run() >= 1
    assert hits.value == 1
    assert lib.qos_timer_active_count() == 0

    periodic_id = lib.qos_timer_add(2, 2, callback, ctypes.cast(ctypes.pointer(hits), ctypes.c_void_p))
    assert periodic_id > 0
    lib.qos_timer_tick(2)
    assert lib.qos_softirq_run() >= 1
    assert hits.value == 2
    assert lib.qos_timer_active_count() == 1

    lib.qos_timer_tick(2)
    assert lib.qos_softirq_run() >= 1
    assert hits.value == 3
    assert lib.qos_timer_cancel(periodic_id) == 0
    assert lib.qos_timer_active_count() == 0


def test_c_timer_rejects_zero_delay() -> None:
    lib = ctypes.CDLL(str(_build_timer_lib()))
    cb_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    lib.qos_timer_reset.argtypes = []
    lib.qos_timer_reset.restype = None
    lib.qos_timer_add.argtypes = [ctypes.c_uint32, ctypes.c_uint32, cb_type, ctypes.c_void_p]
    lib.qos_timer_add.restype = ctypes.c_int32

    @cb_type
    def callback(_ctx: ctypes.c_void_p) -> None:
        return None

    lib.qos_timer_reset()
    assert lib.qos_timer_add(0, 0, callback, None) == -1
