from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QOS_BOOT_MAGIC = 0x514F53424F4F5400
QOS_SIGUSR1 = 10
QOS_SIGUSR2 = 12
QOS_SIG_IGN = 1
HIGH_HANDLER = 0x0000001200001234
SIG_SETMASK = 2


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


class KernelAltStack(ctypes.Structure):
    _fields_ = [
        ("sp", ctypes.c_uint64),
        ("size", ctypes.c_uint64),
        ("flags", ctypes.c_uint32),
        ("_pad", ctypes.c_uint32),
    ]


class HostStackT(ctypes.Structure):
    _fields_ = [
        ("ss_sp", ctypes.c_void_p),
        ("ss_flags", ctypes.c_int),
        ("ss_size", ctypes.c_size_t),
    ]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_kernel_libc_bridge() -> Path:
    out = ROOT / "c-os/build/libqos_c_libc_signal_kernel_bridge.so"
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
            str(ROOT / "c-os/libc/libc.c"),
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


def test_c_libc_signal_calls_route_to_kernel_when_available() -> None:
    lib = ctypes.CDLL(str(_build_kernel_libc_bridge()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_signal_get_handler.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint64)]
    lib.qos_proc_signal_get_handler.restype = ctypes.c_int
    lib.qos_proc_signal_mask.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_mask.restype = ctypes.c_int
    lib.qos_proc_signal_send.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_signal_send.restype = ctypes.c_int
    lib.qos_proc_signal_pending.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_proc_signal_pending.restype = ctypes.c_int

    lib.qos_signal.argtypes = [ctypes.c_int, ctypes.c_void_p]
    lib.qos_signal.restype = ctypes.c_void_p
    lib.qos_sigprocmask.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]
    lib.qos_sigprocmask.restype = ctypes.c_int
    lib.qos_sigpending.argtypes = [ctypes.c_void_p]
    lib.qos_sigpending.restype = ctypes.c_int
    lib.qos_raise.argtypes = [ctypes.c_int]
    lib.qos_raise.restype = ctypes.c_int

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0
    assert lib.qos_proc_create(42, 0) == 0

    prev = lib.qos_signal(QOS_SIGUSR1, ctypes.c_void_p(HIGH_HANDLER))
    assert prev in (None, 0)

    handler = ctypes.c_uint64(0)
    assert lib.qos_proc_signal_get_handler(42, QOS_SIGUSR1, ctypes.byref(handler)) == 0
    assert handler.value == HIGH_HANDLER

    new_mask = ctypes.c_uint32(_bit(QOS_SIGUSR1))
    old_mask = ctypes.c_uint32(0)
    assert lib.qos_sigprocmask(SIG_SETMASK, ctypes.byref(new_mask), ctypes.byref(old_mask)) == 0
    assert old_mask.value == 0

    kernel_mask = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_mask(42, ctypes.byref(kernel_mask)) == 0
    assert kernel_mask.value == _bit(QOS_SIGUSR1)

    assert lib.qos_proc_signal_send(42, QOS_SIGUSR1) == 0
    pending = ctypes.c_uint32(0)
    assert lib.qos_sigpending(ctypes.byref(pending)) == 0
    assert pending.value == _bit(QOS_SIGUSR1)

    assert lib.qos_signal(QOS_SIGUSR2, ctypes.c_void_p(QOS_SIG_IGN)) in (None, 0, QOS_SIG_IGN)
    assert lib.qos_raise(QOS_SIGUSR2) == 0

    kernel_pending = ctypes.c_uint32(0)
    assert lib.qos_proc_signal_pending(42, ctypes.byref(kernel_pending)) == 0
    assert (kernel_pending.value & _bit(QOS_SIGUSR2)) != 0


def test_c_libc_sigaltstack_routes_to_kernel_when_available() -> None:
    lib = ctypes.CDLL(str(_build_kernel_libc_bridge()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_proc_signal_altstack_get.argtypes = [ctypes.c_uint32, ctypes.POINTER(KernelAltStack)]
    lib.qos_proc_signal_altstack_get.restype = ctypes.c_int
    lib.qos_sigaltstack.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    lib.qos_sigaltstack.restype = ctypes.c_int

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0
    assert lib.qos_proc_create(77, 0) == 0

    old_ss = HostStackT()
    new_ss = HostStackT(ctypes.c_void_p(0x900000), 1, 0x4000)
    assert lib.qos_sigaltstack(ctypes.byref(new_ss), ctypes.byref(old_ss)) == 0

    got = KernelAltStack()
    assert lib.qos_proc_signal_altstack_get(77, ctypes.byref(got)) == 0
    assert got.sp == 0x900000
    assert got.size == 0x4000
    assert got.flags == 1

    old_only = HostStackT()
    assert lib.qos_sigaltstack(None, ctypes.byref(old_only)) == 0
    assert old_only.ss_sp == ctypes.c_void_p(0x900000).value
    assert old_only.ss_size == 0x4000
    assert old_only.ss_flags == 1
