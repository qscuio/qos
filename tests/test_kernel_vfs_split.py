from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

QOS_BOOT_MAGIC = 0x514F53424F4F5400
QOS_VFS_FS_INITRAMFS = 1
QOS_VFS_FS_TMPFS = 2
QOS_VFS_FS_PROCFS = 3
QOS_VFS_FS_EXT2 = 4

SYSCALL_NR_OPEN = 5
SYSCALL_NR_READ = 6
SYSCALL_NR_WRITE = 7
SYSCALL_NR_CLOSE = 8
SYSCALL_NR_MKDIR = 22
SYSCALL_NR_UNLINK = 23


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


def _build_vfs_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_vfs_split.so"
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
            str(ROOT / "c-os/kernel/fs/vfs.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def _build_kernel_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_kernel_vfs_split.so"
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


def test_c_vfs_classifies_mount_kinds_and_read_only() -> None:
    lib = ctypes.CDLL(str(_build_vfs_lib()))
    lib.qos_vfs_fs_kind.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_fs_kind.restype = ctypes.c_int
    lib.qos_vfs_is_read_only.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_is_read_only.restype = ctypes.c_int

    assert lib.qos_vfs_fs_kind(b"/init") == QOS_VFS_FS_INITRAMFS
    assert lib.qos_vfs_fs_kind(b"/tmp/work") == QOS_VFS_FS_TMPFS
    assert lib.qos_vfs_fs_kind(b"/proc/meminfo") == QOS_VFS_FS_PROCFS
    assert lib.qos_vfs_fs_kind(b"/data/file") == QOS_VFS_FS_EXT2

    assert lib.qos_vfs_is_read_only(b"/init") == 1
    assert lib.qos_vfs_is_read_only(b"/proc/meminfo") == 1
    assert lib.qos_vfs_is_read_only(b"/tmp/work") == 0
    assert lib.qos_vfs_is_read_only(b"/data/file") == 0
    assert lib.qos_vfs_is_read_only(b"tmp") == -1


def test_c_syscall_procfs_split_behavior() -> None:
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

    meminfo = ctypes.create_string_buffer(b"/proc/meminfo")
    p_meminfo = ctypes.c_uint64(ctypes.addressof(meminfo)).value

    fd = lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, p_meminfo, 0, 0, 0)
    assert fd >= 0

    out = ctypes.create_string_buffer(256)
    out_ptr = ctypes.c_uint64(ctypes.addressof(out)).value
    n = lib.qos_syscall_dispatch(SYSCALL_NR_READ, fd, out_ptr, 256, 0)
    assert n > 0
    text = out.raw[:n]
    assert b"MemTotal:" in text
    assert b"MemFree:" in text
    assert b"ProcCount:" in text

    payload = ctypes.create_string_buffer(b"x")
    payload_ptr = ctypes.c_uint64(ctypes.addressof(payload)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_WRITE, fd, payload_ptr, 1, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd, 0, 0, 0) == 0

    status7 = ctypes.create_string_buffer(b"/proc/7/status")
    p_status7 = ctypes.c_uint64(ctypes.addressof(status7)).value
    fd_status = lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, p_status7, 0, 0, 0)
    assert fd_status >= 0
    out_status = ctypes.create_string_buffer(256)
    out_status_ptr = ctypes.c_uint64(ctypes.addressof(out_status)).value
    m = lib.qos_syscall_dispatch(SYSCALL_NR_READ, fd_status, out_status_ptr, 256, 0)
    assert m > 0
    status_text = out_status.raw[:m]
    assert b"Pid:\t7" in status_text
    assert b"State:\tRunning" in status_text
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd_status, 0, 0, 0) == 0

    status_missing = ctypes.create_string_buffer(b"/proc/999/status")
    p_status_missing = ctypes.c_uint64(ctypes.addressof(status_missing)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, p_status_missing, 0, 0, 0) == -1

    proc_new = ctypes.create_string_buffer(b"/proc/new")
    p_proc_new = ctypes.c_uint64(ctypes.addressof(proc_new)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MKDIR, p_proc_new, 0, 0, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_UNLINK, p_meminfo, 0, 0, 0) == -1

    data_path = ctypes.create_string_buffer(b"/data/test")
    p_data = ctypes.c_uint64(ctypes.addressof(data_path)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MKDIR, p_data, 0, 0, 0) == 0
    fd_data = lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, p_data, 0, 0, 0)
    assert fd_data >= 0
    data = ctypes.create_string_buffer(b"ok")
    p_data_buf = ctypes.c_uint64(ctypes.addressof(data)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_WRITE, fd_data, p_data_buf, 2, 0) == 2
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd_data, 0, 0, 0) == 0
