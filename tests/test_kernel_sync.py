from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class SpinLock(ctypes.Structure):
    _fields_ = [("locked", ctypes.c_uint32)]


class Mutex(ctypes.Structure):
    _fields_ = [("locked", ctypes.c_uint32), ("waiters", ctypes.c_uint32)]


class Semaphore(ctypes.Structure):
    _fields_ = [("value", ctypes.c_int32), ("waiters", ctypes.c_uint32)]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_sync_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_sync.so"
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
            str(ROOT / "c-os/kernel/sync/sync.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_spinlock_flow() -> None:
    lib = ctypes.CDLL(str(_build_sync_lib()))
    lib.qos_spin_init.argtypes = [ctypes.POINTER(SpinLock)]
    lib.qos_spin_init.restype = ctypes.c_int
    lib.qos_spin_trylock.argtypes = [ctypes.POINTER(SpinLock)]
    lib.qos_spin_trylock.restype = ctypes.c_int
    lib.qos_spin_lock.argtypes = [ctypes.POINTER(SpinLock)]
    lib.qos_spin_lock.restype = ctypes.c_int
    lib.qos_spin_unlock.argtypes = [ctypes.POINTER(SpinLock)]
    lib.qos_spin_unlock.restype = ctypes.c_int
    lib.qos_spin_is_locked.argtypes = [ctypes.POINTER(SpinLock)]
    lib.qos_spin_is_locked.restype = ctypes.c_int

    lock = SpinLock()
    assert lib.qos_spin_init(ctypes.byref(lock)) == 0
    assert lib.qos_spin_is_locked(ctypes.byref(lock)) == 0
    assert lib.qos_spin_trylock(ctypes.byref(lock)) == 0
    assert lib.qos_spin_is_locked(ctypes.byref(lock)) == 1
    assert lib.qos_spin_trylock(ctypes.byref(lock)) != 0
    assert lib.qos_spin_unlock(ctypes.byref(lock)) == 0
    assert lib.qos_spin_is_locked(ctypes.byref(lock)) == 0
    assert lib.qos_spin_lock(ctypes.byref(lock)) == 0
    assert lib.qos_spin_is_locked(ctypes.byref(lock)) == 1


def test_c_mutex_and_semaphore_flow() -> None:
    lib = ctypes.CDLL(str(_build_sync_lib()))

    lib.qos_mutex_init.argtypes = [ctypes.POINTER(Mutex)]
    lib.qos_mutex_init.restype = ctypes.c_int
    lib.qos_mutex_lock.argtypes = [ctypes.POINTER(Mutex)]
    lib.qos_mutex_lock.restype = ctypes.c_int
    lib.qos_mutex_trylock.argtypes = [ctypes.POINTER(Mutex)]
    lib.qos_mutex_trylock.restype = ctypes.c_int
    lib.qos_mutex_unlock.argtypes = [ctypes.POINTER(Mutex)]
    lib.qos_mutex_unlock.restype = ctypes.c_int
    lib.qos_mutex_waiters.argtypes = [ctypes.POINTER(Mutex)]
    lib.qos_mutex_waiters.restype = ctypes.c_uint32
    lib.qos_mutex_is_locked.argtypes = [ctypes.POINTER(Mutex)]
    lib.qos_mutex_is_locked.restype = ctypes.c_int

    m = Mutex()
    assert lib.qos_mutex_init(ctypes.byref(m)) == 0
    assert lib.qos_mutex_lock(ctypes.byref(m)) == 0
    assert lib.qos_mutex_trylock(ctypes.byref(m)) != 0
    assert lib.qos_mutex_lock(ctypes.byref(m)) != 0
    assert lib.qos_mutex_waiters(ctypes.byref(m)) == 1
    assert lib.qos_mutex_unlock(ctypes.byref(m)) == 0
    assert lib.qos_mutex_waiters(ctypes.byref(m)) == 0
    assert lib.qos_mutex_is_locked(ctypes.byref(m)) == 0
    assert lib.qos_mutex_lock(ctypes.byref(m)) == 0
    assert lib.qos_mutex_is_locked(ctypes.byref(m)) == 1
    assert lib.qos_mutex_unlock(ctypes.byref(m)) == 0
    assert lib.qos_mutex_is_locked(ctypes.byref(m)) == 0

    lib.qos_sem_init.argtypes = [ctypes.POINTER(Semaphore), ctypes.c_int32]
    lib.qos_sem_init.restype = ctypes.c_int
    lib.qos_sem_wait.argtypes = [ctypes.POINTER(Semaphore)]
    lib.qos_sem_wait.restype = ctypes.c_int
    lib.qos_sem_post.argtypes = [ctypes.POINTER(Semaphore)]
    lib.qos_sem_post.restype = ctypes.c_int
    lib.qos_sem_value.argtypes = [ctypes.POINTER(Semaphore)]
    lib.qos_sem_value.restype = ctypes.c_int32
    lib.qos_sem_waiters.argtypes = [ctypes.POINTER(Semaphore)]
    lib.qos_sem_waiters.restype = ctypes.c_uint32

    s = Semaphore()
    assert lib.qos_sem_init(ctypes.byref(s), 1) == 0
    assert lib.qos_sem_wait(ctypes.byref(s)) == 0
    assert lib.qos_sem_value(ctypes.byref(s)) == 0
    assert lib.qos_sem_wait(ctypes.byref(s)) != 0
    assert lib.qos_sem_waiters(ctypes.byref(s)) == 1
    assert lib.qos_sem_post(ctypes.byref(s)) == 1
    assert lib.qos_sem_waiters(ctypes.byref(s)) == 0
    assert lib.qos_sem_value(ctypes.byref(s)) == 0
    assert lib.qos_sem_post(ctypes.byref(s)) == 0
    assert lib.qos_sem_value(ctypes.byref(s)) == 1
