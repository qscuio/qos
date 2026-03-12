from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

QOS_BOOT_MAGIC = 0x514F53424F4F5400
SYSCALL_NR_SIGACTION = 30
SYSCALL_NR_SIGPROCMASK = 31
QOS_SIGUSR1 = 10
QOS_SIGUSR2 = 12
QOS_SIGKILL = 9
QOS_SIG_IGN = 1
HIGH_HANDLER = 0x0000001200001234
SIG_BLOCK = 0
SIG_UNBLOCK = 1
SIG_SETMASK = 2
U64_MAX = (1 << 64) - 1


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


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_kernel_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_kernel_syscall_signal_ctl.so"
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


def test_c_syscall_sigaction_and_sigprocmask_bridge() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_signal_mask.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_mask.restype = ctypes.c_int
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
    assert lib.qos_proc_create(7, 0) == 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_SIGACTION, 7, QOS_SIGUSR1, U64_MAX, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SIGACTION, 7, QOS_SIGUSR1, QOS_SIG_IGN, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SIGACTION, 7, QOS_SIGUSR1, U64_MAX, 0) == QOS_SIG_IGN
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SIGACTION, 7, QOS_SIGUSR2, HIGH_HANDLER, 0) == 0
    got = lib.qos_syscall_dispatch(SYSCALL_NR_SIGACTION, 7, QOS_SIGUSR2, U64_MAX, 0)
    assert ctypes.c_uint64(got).value == HIGH_HANDLER
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SIGACTION, 7, QOS_SIGKILL, 0x1234, 0) == -1

    assert lib.qos_syscall_dispatch(SYSCALL_NR_SIGPROCMASK, 7, SIG_SETMASK, _bit(QOS_SIGUSR1), 0) == 0
    assert (
        lib.qos_syscall_dispatch(SYSCALL_NR_SIGPROCMASK, 7, SIG_BLOCK, _bit(QOS_SIGUSR2), 0)
        == _bit(QOS_SIGUSR1)
    )

    current = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_mask(7, ctypes.byref(current)) == 0
    assert current.value == (_bit(QOS_SIGUSR1) | _bit(QOS_SIGUSR2))

    assert (
        lib.qos_syscall_dispatch(SYSCALL_NR_SIGPROCMASK, 7, SIG_UNBLOCK, _bit(QOS_SIGUSR1), 0)
        == (_bit(QOS_SIGUSR1) | _bit(QOS_SIGUSR2))
    )
    assert lib.qos_proc_signal_mask(7, ctypes.byref(current)) == 0
    assert current.value == _bit(QOS_SIGUSR2)

    assert lib.qos_syscall_dispatch(SYSCALL_NR_SIGPROCMASK, 7, 99, 0, 0) == -1
