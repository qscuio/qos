from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

QOS_BOOT_MAGIC = 0x514F53424F4F5400
SYSCALL_NR_OPEN = 5
SYSCALL_NR_READ = 6
SYSCALL_NR_CLOSE = 8
SYSCALL_NR_STAT = 21


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
    out = ROOT / "c-os/build/libqos_c_kernel_procfs.so"
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


def _read_path(lib: ctypes.CDLL, path: bytes) -> str:
    p = ctypes.create_string_buffer(path)
    p_ptr = ctypes.c_uint64(ctypes.addressof(p)).value
    fd = lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, p_ptr, 0, 0, 0)
    assert fd >= 0, path
    out = ctypes.create_string_buffer(1024)
    out_ptr = ctypes.c_uint64(ctypes.addressof(out)).value
    n = lib.qos_syscall_dispatch(SYSCALL_NR_READ, fd, out_ptr, 1023, 0)
    assert n >= 0, path
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd, 0, 0, 0) == 0
    return bytes(out.raw[: int(n)]).decode("utf-8", errors="replace")


def test_c_procfs_exposes_kernel_information_files() -> None:
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
    assert lib.qos_proc_create(7, 0) == 0

    meminfo = _read_path(lib, b"/proc/meminfo")
    assert "MemTotal:" in meminfo
    assert "MemFree:" in meminfo

    kstatus = _read_path(lib, b"/proc/kernel/status")
    assert "InitState:" in kstatus
    assert "Syscalls:" in kstatus
    assert "ProcCount:" in kstatus

    netdev = _read_path(lib, b"/proc/net/dev")
    assert "eth0:" in netdev
    assert "Receive" in netdev

    syscalls = _read_path(lib, b"/proc/syscalls")
    assert "SyscallCount:" in syscalls

    uptime = _read_path(lib, b"/proc/uptime")
    assert "0.00 0.00" in uptime

    status7 = _read_path(lib, b"/proc/7/status")
    assert "Pid:\t7" in status7
    assert "State:\tRunning" in status7

    stat_path = ctypes.create_string_buffer(b"/proc/kernel/status")
    stat_path_ptr = ctypes.c_uint64(ctypes.addressof(stat_path)).value
    stat_out = (ctypes.c_uint64 * 2)(0, 0)
    stat_out_ptr = ctypes.c_uint64(ctypes.addressof(stat_out)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_STAT, stat_path_ptr, stat_out_ptr, 0, 0) == 0
    assert stat_out[1] == 2
