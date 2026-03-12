from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

QOS_BOOT_MAGIC = 0x514F53424F4F5400
SYSCALL_NR_FORK = 0
SYSCALL_NR_EXEC = 1
QOS_SIG_DFL = 0
QOS_SIG_IGN = 1
QOS_SIGUSR1 = 10
QOS_SIGUSR2 = 12


class MmapEntry(ctypes.Structure):
    _fields_ = [
        ("base", ctypes.c_uint64),
        ("length", ctypes.c_uint64),
        ("type", ctypes.c_uint32),
        ("_pad", ctypes.c_uint32),
    ]


class BootInfo(ctypes.Structure):
    _fields_ = [
        ("magic", ctypes.c_uint64),
        ("mmap_entry_count", ctypes.c_uint32),
        ("_pad0", ctypes.c_uint32),
        ("mmap_entries", MmapEntry * 128),
        ("fb_addr", ctypes.c_uint64),
        ("fb_width", ctypes.c_uint32),
        ("fb_height", ctypes.c_uint32),
        ("fb_pitch", ctypes.c_uint32),
        ("fb_bpp", ctypes.c_uint8),
        ("_pad1", ctypes.c_uint8 * 3),
        ("initramfs_addr", ctypes.c_uint64),
        ("initramfs_size", ctypes.c_uint64),
        ("acpi_rsdp_addr", ctypes.c_uint64),
        ("dtb_addr", ctypes.c_uint64),
    ]


class AltStack(ctypes.Structure):
    _fields_ = [
        ("sp", ctypes.c_uint64),
        ("size", ctypes.c_uint64),
        ("flags", ctypes.c_uint32),
        ("_pad", ctypes.c_uint32),
    ]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_kernel_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_kernel_syscall_proc_lifecycle.so"
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
            str(ROOT / "c-os/kernel/kernel.c"),
            str(ROOT / "c-os/kernel/init_state.c"),
            str(ROOT / "c-os/kernel/mm/mm.c"),
            str(ROOT / "c-os/kernel/sched/sched.c"),
            str(ROOT / "c-os/kernel/fs/vfs.c"),
            str(ROOT / "c-os/kernel/net/net.c"),
            str(ROOT / "c-os/kernel/drivers/drivers.c"),
            str(ROOT / "c-os/kernel/syscall/syscall.c"),
            str(ROOT / "c-os/kernel/proc/proc.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def _valid_boot_info() -> BootInfo:
    info = BootInfo()
    info.magic = QOS_BOOT_MAGIC
    info.mmap_entry_count = 1
    info.mmap_entries[0].base = 0x100000
    info.mmap_entries[0].length = 0x200000
    info.mmap_entries[0].type = 1
    info.initramfs_addr = 0x400000
    info.initramfs_size = 0x1000
    return info


def _bit(signum: int) -> int:
    return 1 << signum


def test_c_syscall_fork_exec_bridge_and_signal_semantics() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_parent.argtypes = [ctypes.c_uint32]
    lib.qos_proc_parent.restype = ctypes.c_int32
    lib.qos_proc_alive.argtypes = [ctypes.c_uint32]
    lib.qos_proc_alive.restype = ctypes.c_int
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
    lib.qos_proc_signal_send.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_send.restype = ctypes.c_int
    lib.qos_proc_signal_pending.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_pending.restype = ctypes.c_int
    lib.qos_proc_signal_altstack_set.argtypes = [ctypes.c_uint32, ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_proc_signal_altstack_set.restype = ctypes.c_int
    lib.qos_proc_signal_altstack_get.argtypes = [ctypes.c_uint32, ctypes.POINTER(AltStack)]
    lib.qos_proc_signal_altstack_get.restype = ctypes.c_int
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0
    assert lib.qos_proc_create(1, 0) == 0
    assert lib.qos_proc_signal_set_handler(1, QOS_SIGUSR1, QOS_SIG_IGN) == 0
    assert lib.qos_proc_signal_set_handler(1, QOS_SIGUSR2, 0x2000) == 0
    assert lib.qos_proc_signal_set_mask(1, _bit(QOS_SIGUSR2)) == 0
    assert lib.qos_proc_signal_altstack_set(1, 0x800000, 0x3000, 2) == 0
    assert lib.qos_proc_signal_send(1, QOS_SIGUSR2) == 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_FORK, 1, 2, 0, 0) == 2
    assert lib.qos_proc_alive(2) == 1
    assert lib.qos_proc_parent(2) == 1

    h_usr1 = ctypes.c_uint64(0)
    h_usr2 = ctypes.c_uint64(0)
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR1, ctypes.byref(h_usr1)) == 0
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR2, ctypes.byref(h_usr2)) == 0
    assert h_usr1.value == QOS_SIG_IGN
    assert h_usr2.value == 0x2000

    child_mask = ctypes.c_uint32(0)
    child_pending = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_mask(2, ctypes.byref(child_mask)) == 0
    assert lib.qos_proc_signal_pending(2, ctypes.byref(child_pending)) == 0
    assert child_mask.value == _bit(QOS_SIGUSR2)
    assert child_pending.value == 0

    child_alt = AltStack()
    assert lib.qos_proc_signal_altstack_get(2, ctypes.byref(child_alt)) == 0
    assert child_alt.sp == 0x800000
    assert child_alt.size == 0x3000
    assert child_alt.flags == 2

    assert lib.qos_proc_signal_send(2, QOS_SIGUSR2) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_EXEC, 2, 0, 0, 0) == 0

    h_usr1_after = ctypes.c_uint64(0)
    h_usr2_after = ctypes.c_uint64(0)
    child_pending_after = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR1, ctypes.byref(h_usr1_after)) == 0
    assert lib.qos_proc_signal_get_handler(2, QOS_SIGUSR2, ctypes.byref(h_usr2_after)) == 0
    assert lib.qos_proc_signal_pending(2, ctypes.byref(child_pending_after)) == 0
    assert h_usr1_after.value == QOS_SIG_IGN
    assert h_usr2_after.value == QOS_SIG_DFL
    assert child_pending_after.value == 0


def test_c_syscall_fork_exec_reject_invalid_inputs() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0
    assert lib.qos_proc_create(1, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_FORK, 999, 2, 0, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_FORK, 1, 0, 0, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_EXEC, 999, 0, 0, 0) == -1
