from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_napi_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_napi.so"
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
            str(ROOT / "c-os/kernel/net/napi.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_napi_schedule_poll_and_complete() -> None:
    lib = ctypes.CDLL(str(_build_napi_lib()))
    poll_type = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32)

    lib.qos_softirq_reset.argtypes = []
    lib.qos_softirq_reset.restype = None
    lib.qos_softirq_pending_mask.argtypes = []
    lib.qos_softirq_pending_mask.restype = ctypes.c_uint32
    lib.qos_softirq_run.argtypes = []
    lib.qos_softirq_run.restype = ctypes.c_uint32

    lib.qos_napi_reset.argtypes = []
    lib.qos_napi_reset.restype = None
    lib.qos_napi_register.argtypes = [ctypes.c_uint32, poll_type, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_napi_register.restype = ctypes.c_int
    lib.qos_napi_schedule.argtypes = [ctypes.c_uint32]
    lib.qos_napi_schedule.restype = ctypes.c_int
    lib.qos_napi_complete.argtypes = [ctypes.c_uint32]
    lib.qos_napi_complete.restype = ctypes.c_int
    lib.qos_napi_pending_count.argtypes = []
    lib.qos_napi_pending_count.restype = ctypes.c_uint32
    lib.qos_napi_run_count.argtypes = [ctypes.c_uint32]
    lib.qos_napi_run_count.restype = ctypes.c_uint32
    lib.napi_init.argtypes = []
    lib.napi_init.restype = None

    state = {"calls": 0}

    @poll_type
    def poll(ctx: ctypes.c_void_p, budget: int) -> int:
        state["calls"] += 1
        mode_ptr = ctypes.cast(ctx, ctypes.POINTER(ctypes.c_uint32))
        mode = mode_ptr[0]
        if mode >= budget:
            mode_ptr[0] = budget - 1
            return budget
        return mode

    mode = ctypes.c_uint32(2)
    napi_id = ctypes.c_uint32(0)

    lib.qos_softirq_reset()
    lib.qos_napi_reset()
    lib.napi_init()

    assert lib.qos_napi_register(4, poll, ctypes.cast(ctypes.pointer(mode), ctypes.c_void_p), ctypes.byref(napi_id)) == 0
    assert napi_id.value != 0

    assert lib.qos_napi_schedule(napi_id.value) == 0
    assert lib.qos_napi_pending_count() == 1
    assert (lib.qos_softirq_pending_mask() & (1 << 1)) != 0

    assert lib.qos_softirq_run() >= 1
    assert state["calls"] == 1
    assert lib.qos_napi_run_count(napi_id.value) == 1
    assert lib.qos_napi_pending_count() == 0

    mode.value = 4
    assert lib.qos_napi_schedule(napi_id.value) == 0
    assert lib.qos_softirq_run() >= 1
    assert state["calls"] == 3
    assert lib.qos_napi_run_count(napi_id.value) == 3
    assert lib.qos_napi_pending_count() == 0
    assert lib.qos_napi_complete(napi_id.value) == 0


def test_c_napi_rejects_invalid_registration() -> None:
    lib = ctypes.CDLL(str(_build_napi_lib()))
    poll_type = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32)

    lib.qos_napi_reset.argtypes = []
    lib.qos_napi_reset.restype = None
    lib.qos_napi_register.argtypes = [ctypes.c_uint32, poll_type, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_napi_register.restype = ctypes.c_int

    @poll_type
    def poll(_ctx: ctypes.c_void_p, _budget: int) -> int:
        return 0

    napi_id = ctypes.c_uint32(0)
    lib.qos_napi_reset()
    assert lib.qos_napi_register(0, poll, None, ctypes.byref(napi_id)) == -1
