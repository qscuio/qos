from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

QOS_SIG_DFL = 0
QOS_SIG_IGN = 1
QOS_SIGKILL = 9
QOS_SIGUSR1 = 10
QOS_SIGUSR2 = 12
QOS_SIGTERM = 15
QOS_SIGCONT = 18
QOS_SIGSTOP = 19
HIGH_HANDLER = 0x0000001200001234


class AltStack(ctypes.Structure):
    _fields_ = [
        ("sp", ctypes.c_uint64),
        ("size", ctypes.c_uint64),
        ("flags", ctypes.c_uint32),
        ("_pad", ctypes.c_uint32),
    ]


class SigFrame(ctypes.Structure):
    _fields_ = [
        ("pretcode", ctypes.c_uint64),
        ("signum", ctypes.c_int32),
        ("saved_mask", ctypes.c_uint32),
        ("saved_handler", ctypes.c_uint64),
    ]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_proc_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_proc_signal.so"
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


def _bit(signum: int) -> int:
    return 1 << signum


def test_c_proc_signal_mask_pending_and_delivery() -> None:
    lib = ctypes.CDLL(str(_build_proc_lib()))
    lib.qos_proc_reset.argtypes = []
    lib.qos_proc_reset.restype = None
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_signal_set_handler.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_proc_signal_set_handler.restype = ctypes.c_int
    lib.qos_proc_signal_get_handler.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_uint64),
    ]
    lib.qos_proc_signal_get_handler.restype = ctypes.c_int
    lib.qos_proc_signal_set_mask.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_set_mask.restype = ctypes.c_int
    lib.qos_proc_signal_mask.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_mask.restype = ctypes.c_int
    lib.qos_proc_signal_pending.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_pending.restype = ctypes.c_int
    lib.qos_proc_signal_send.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_send.restype = ctypes.c_int
    lib.qos_proc_signal_next.argtypes = [ctypes.c_uint32]
    lib.qos_proc_signal_next.restype = ctypes.c_int32

    lib.qos_proc_reset()
    assert lib.qos_proc_create(10, 0) == 0

    # SIGKILL/SIGSTOP cannot be caught or ignored.
    assert lib.qos_proc_signal_set_handler(10, QOS_SIGKILL, 0x1234) != 0

    assert lib.qos_proc_signal_set_handler(10, QOS_SIGUSR1, HIGH_HANDLER) == 0
    out_handler = ctypes.c_uint64(0)
    assert lib.qos_proc_signal_get_handler(10, QOS_SIGUSR1, ctypes.byref(out_handler)) == 0
    assert out_handler.value == HIGH_HANDLER

    # Blocking SIGKILL/SIGSTOP must be ignored.
    mask = _bit(QOS_SIGUSR1) | _bit(QOS_SIGKILL) | _bit(QOS_SIGSTOP)
    assert lib.qos_proc_signal_set_mask(10, mask) == 0
    out_mask = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_mask(10, ctypes.byref(out_mask)) == 0
    assert out_mask.value == _bit(QOS_SIGUSR1)

    assert lib.qos_proc_signal_send(10, QOS_SIGUSR1) == 0
    assert lib.qos_proc_signal_send(10, QOS_SIGKILL) == 0

    out_pending = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_pending(10, ctypes.byref(out_pending)) == 0
    assert (out_pending.value & _bit(QOS_SIGUSR1)) != 0
    assert (out_pending.value & _bit(QOS_SIGKILL)) != 0

    # SIGKILL must be delivered even when masked.
    assert lib.qos_proc_signal_next(10) == QOS_SIGKILL
    # SIGUSR1 still pending but masked.
    assert lib.qos_proc_signal_next(10) == 0

    assert lib.qos_proc_signal_set_mask(10, 0) == 0
    assert lib.qos_proc_signal_next(10) == QOS_SIGUSR1
    assert lib.qos_proc_signal_next(10) == 0


def test_c_proc_signal_fork_and_exec_semantics() -> None:
    lib = ctypes.CDLL(str(_build_proc_lib()))
    lib.qos_proc_reset.argtypes = []
    lib.qos_proc_reset.restype = None
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_fork.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_fork.restype = ctypes.c_int
    lib.qos_proc_exec_signal_reset.argtypes = [ctypes.c_uint32]
    lib.qos_proc_exec_signal_reset.restype = ctypes.c_int
    lib.qos_proc_signal_set_handler.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_proc_signal_set_handler.restype = ctypes.c_int
    lib.qos_proc_signal_get_handler.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_uint64),
    ]
    lib.qos_proc_signal_get_handler.restype = ctypes.c_int
    lib.qos_proc_signal_set_mask.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_set_mask.restype = ctypes.c_int
    lib.qos_proc_signal_mask.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_mask.restype = ctypes.c_int
    lib.qos_proc_signal_pending.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_pending.restype = ctypes.c_int
    lib.qos_proc_signal_send.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_send.restype = ctypes.c_int
    lib.qos_proc_signal_altstack_set.argtypes = [ctypes.c_uint32, ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_proc_signal_altstack_set.restype = ctypes.c_int
    lib.qos_proc_signal_altstack_get.argtypes = [ctypes.c_uint32, ctypes.POINTER(AltStack)]
    lib.qos_proc_signal_altstack_get.restype = ctypes.c_int

    lib.qos_proc_reset()
    assert lib.qos_proc_create(1, 0) == 0
    assert lib.qos_proc_signal_set_handler(1, QOS_SIGUSR1, QOS_SIG_IGN) == 0
    assert lib.qos_proc_signal_set_handler(1, QOS_SIGUSR2, 0x2000) == 0
    assert lib.qos_proc_signal_altstack_set(1, 0x800000, 0x4000, 1) == 0
    assert lib.qos_proc_signal_set_mask(1, _bit(QOS_SIGUSR2)) == 0
    assert lib.qos_proc_signal_send(1, QOS_SIGUSR2) == 0

    assert lib.qos_proc_fork(1, 2) == 0

    child_pending = ctypes.c_uint32(0)
    child_mask = ctypes.c_uint32(0)
    h_usr1 = ctypes.c_uint64(0)
    h_usr2 = ctypes.c_uint64(0)
    assert lib.qos_proc_signal_pending(2, ctypes.byref(child_pending)) == 0
    assert child_pending.value == 0
    assert lib.qos_proc_signal_mask(2, ctypes.byref(child_mask)) == 0
    assert child_mask.value == _bit(QOS_SIGUSR2)
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR1, ctypes.byref(h_usr1)) == 0
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR2, ctypes.byref(h_usr2)) == 0
    assert h_usr1.value == QOS_SIG_IGN
    assert h_usr2.value == 0x2000
    child_alt = AltStack()
    assert lib.qos_proc_signal_altstack_get(2, ctypes.byref(child_alt)) == 0
    assert child_alt.sp == 0x800000
    assert child_alt.size == 0x4000
    assert child_alt.flags == 1

    assert lib.qos_proc_exec_signal_reset(2) == 0

    child_pending2 = ctypes.c_uint32(0)
    child_mask2 = ctypes.c_uint32(0)
    h_usr1_after = ctypes.c_uint64(0)
    h_usr2_after = ctypes.c_uint64(0)
    assert lib.qos_proc_signal_pending(2, ctypes.byref(child_pending2)) == 0
    assert child_pending2.value == 0
    assert lib.qos_proc_signal_mask(2, ctypes.byref(child_mask2)) == 0
    assert child_mask2.value == _bit(QOS_SIGUSR2)
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR1, ctypes.byref(h_usr1_after)) == 0
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR2, ctypes.byref(h_usr2_after)) == 0
    assert h_usr1_after.value == QOS_SIG_IGN
    assert h_usr2_after.value == QOS_SIG_DFL


def test_c_proc_signal_default_actions_and_frame_helpers() -> None:
    lib = ctypes.CDLL(str(_build_proc_lib()))
    lib.qos_proc_reset.argtypes = []
    lib.qos_proc_reset.restype = None
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_alive.argtypes = [ctypes.c_uint32]
    lib.qos_proc_alive.restype = ctypes.c_int
    lib.qos_proc_is_stopped.argtypes = [ctypes.c_uint32]
    lib.qos_proc_is_stopped.restype = ctypes.c_int
    lib.qos_proc_signal_send.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_send.restype = ctypes.c_int
    lib.qos_proc_signal_next.argtypes = [ctypes.c_uint32]
    lib.qos_proc_signal_next.restype = ctypes.c_int32
    lib.qos_proc_signal_set_handler.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_proc_signal_set_handler.restype = ctypes.c_int
    lib.qos_proc_signal_set_mask.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_set_mask.restype = ctypes.c_int
    lib.qos_proc_signal_prepare_frame.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(SigFrame)]
    lib.qos_proc_signal_prepare_frame.restype = ctypes.c_int
    lib.qos_proc_signal_sigreturn.argtypes = [ctypes.c_uint32, ctypes.POINTER(SigFrame)]
    lib.qos_proc_signal_sigreturn.restype = ctypes.c_int
    lib.qos_proc_signal_mask.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_mask.restype = ctypes.c_int

    lib.qos_proc_reset()

    assert lib.qos_proc_create(41, 0) == 0
    assert lib.qos_proc_signal_send(41, QOS_SIGTERM) == 0
    assert lib.qos_proc_signal_next(41) == QOS_SIGTERM
    assert lib.qos_proc_alive(41) == 0

    assert lib.qos_proc_create(42, 0) == 0
    assert lib.qos_proc_signal_send(42, QOS_SIGSTOP) == 0
    assert lib.qos_proc_signal_next(42) == QOS_SIGSTOP
    assert lib.qos_proc_is_stopped(42) == 1
    assert lib.qos_proc_signal_send(42, QOS_SIGCONT) == 0
    assert lib.qos_proc_signal_next(42) == QOS_SIGCONT
    assert lib.qos_proc_is_stopped(42) == 0

    assert lib.qos_proc_create(43, 0) == 0
    assert lib.qos_proc_signal_set_handler(43, QOS_SIGUSR1, QOS_SIG_IGN) == 0
    assert lib.qos_proc_signal_send(43, QOS_SIGUSR1) == 0
    assert lib.qos_proc_signal_next(43) == 0

    assert lib.qos_proc_create(44, 0) == 0
    assert lib.qos_proc_signal_set_handler(44, QOS_SIGUSR1, HIGH_HANDLER) == 0
    assert lib.qos_proc_signal_set_mask(44, _bit(QOS_SIGUSR2)) == 0
    frame = SigFrame()
    assert lib.qos_proc_signal_prepare_frame(44, QOS_SIGUSR1, ctypes.byref(frame)) == 0
    assert frame.pretcode == 0x00007FFF00000000
    assert frame.signum == QOS_SIGUSR1
    assert frame.saved_handler == HIGH_HANDLER
    assert frame.saved_mask == _bit(QOS_SIGUSR2)

    assert lib.qos_proc_signal_set_mask(44, _bit(QOS_SIGUSR1)) == 0
    assert lib.qos_proc_signal_sigreturn(44, ctypes.byref(frame)) == 0
    restored = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_mask(44, ctypes.byref(restored)) == 0
    assert restored.value == _bit(QOS_SIGUSR2)
