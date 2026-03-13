from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_sched_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_sched.so"
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
            str(ROOT / "c-os/kernel/sched/sched.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_sched_round_robin_order() -> None:
    lib = ctypes.CDLL(str(_build_sched_lib()))
    lib.qos_sched_reset.argtypes = []
    lib.qos_sched_reset.restype = None
    lib.qos_sched_add_task.argtypes = [ctypes.c_uint32]
    lib.qos_sched_add_task.restype = ctypes.c_int
    lib.qos_sched_next.argtypes = []
    lib.qos_sched_next.restype = ctypes.c_uint32
    lib.qos_sched_count.argtypes = []
    lib.qos_sched_count.restype = ctypes.c_uint32
    lib.qos_sched_current.argtypes = []
    lib.qos_sched_current.restype = ctypes.c_uint32

    lib.qos_sched_reset()
    assert lib.qos_sched_current() == 0
    assert lib.qos_sched_add_task(1) == 0
    assert lib.qos_sched_add_task(2) == 0
    assert lib.qos_sched_add_task(3) == 0
    assert lib.qos_sched_count() == 3
    assert [lib.qos_sched_next() for _ in range(4)] == [1, 2, 3, 1]
    assert lib.qos_sched_current() == 1


def test_c_sched_remove_and_duplicate_rules() -> None:
    lib = ctypes.CDLL(str(_build_sched_lib()))
    lib.qos_sched_reset.argtypes = []
    lib.qos_sched_reset.restype = None
    lib.qos_sched_add_task.argtypes = [ctypes.c_uint32]
    lib.qos_sched_add_task.restype = ctypes.c_int
    lib.qos_sched_remove_task.argtypes = [ctypes.c_uint32]
    lib.qos_sched_remove_task.restype = ctypes.c_int
    lib.qos_sched_next.argtypes = []
    lib.qos_sched_next.restype = ctypes.c_uint32

    lib.qos_sched_reset()
    assert lib.qos_sched_add_task(7) == 0
    assert lib.qos_sched_add_task(7) != 0
    assert lib.qos_sched_add_task(8) == 0
    assert lib.qos_sched_remove_task(7) == 0
    assert lib.qos_sched_remove_task(7) != 0
    assert lib.qos_sched_next() == 8


def test_c_sched_priority_levels_and_round_robin_within_level() -> None:
    lib = ctypes.CDLL(str(_build_sched_lib()))
    lib.qos_sched_reset.argtypes = []
    lib.qos_sched_reset.restype = None
    lib.qos_sched_add_task_prio.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_sched_add_task_prio.restype = ctypes.c_int
    lib.qos_sched_remove_task.argtypes = [ctypes.c_uint32]
    lib.qos_sched_remove_task.restype = ctypes.c_int
    lib.qos_sched_next.argtypes = []
    lib.qos_sched_next.restype = ctypes.c_uint32
    lib.qos_sched_get_priority.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_sched_get_priority.restype = ctypes.c_int
    lib.qos_sched_set_priority.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_sched_set_priority.restype = ctypes.c_int

    # Higher priority tasks are always selected first; same-priority tasks round-robin.
    lib.qos_sched_reset()
    assert lib.qos_sched_add_task_prio(10, 3) == 0
    assert lib.qos_sched_add_task_prio(11, 3) == 0
    assert lib.qos_sched_add_task_prio(20, 2) == 0
    assert [lib.qos_sched_next() for _ in range(4)] == [10, 11, 10, 11]

    prio = ctypes.c_uint32(0)
    assert lib.qos_sched_get_priority(20, ctypes.byref(prio)) == 0
    assert prio.value == 2
    assert lib.qos_sched_set_priority(20, 3) == 0
    assert [lib.qos_sched_next() for _ in range(3)] == [10, 11, 20]

    assert lib.qos_sched_remove_task(10) == 0
    assert lib.qos_sched_remove_task(11) == 0
    assert lib.qos_sched_next() == 20


def test_c_sched_priority_levels_and_reassignment() -> None:
    lib = ctypes.CDLL(str(_build_sched_lib()))
    lib.qos_sched_reset.argtypes = []
    lib.qos_sched_reset.restype = None
    lib.qos_sched_add_task.argtypes = [ctypes.c_uint32]
    lib.qos_sched_add_task.restype = ctypes.c_int
    lib.qos_sched_add_task_prio.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_sched_add_task_prio.restype = ctypes.c_int
    lib.qos_sched_set_priority.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_sched_set_priority.restype = ctypes.c_int
    lib.qos_sched_get_priority.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_sched_get_priority.restype = ctypes.c_int
    lib.qos_sched_next.argtypes = []
    lib.qos_sched_next.restype = ctypes.c_uint32

    lib.qos_sched_reset()
    assert lib.qos_sched_add_task(100) == 0  # default priority = normal
    assert lib.qos_sched_add_task_prio(200, 3) == 0  # high
    assert lib.qos_sched_add_task_prio(300, 1) == 0  # low

    p100 = ctypes.c_uint32(0)
    p200 = ctypes.c_uint32(0)
    p300 = ctypes.c_uint32(0)
    assert lib.qos_sched_get_priority(100, ctypes.byref(p100)) == 0
    assert lib.qos_sched_get_priority(200, ctypes.byref(p200)) == 0
    assert lib.qos_sched_get_priority(300, ctypes.byref(p300)) == 0
    assert p100.value == 2
    assert p200.value == 3
    assert p300.value == 1

    # Higher-priority queue must always run first, round-robin within each level.
    assert [lib.qos_sched_next() for _ in range(4)] == [200, 200, 200, 200]

    assert lib.qos_sched_set_priority(100, 3) == 0
    assert lib.qos_sched_get_priority(100, ctypes.byref(p100)) == 0
    assert p100.value == 3
    assert [lib.qos_sched_next() for _ in range(6)] == [200, 100, 200, 100, 200, 100]
