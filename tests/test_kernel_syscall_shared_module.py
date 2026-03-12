from __future__ import annotations

import ctypes
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

QOS_BOOT_MAGIC = 0x514F53424F4F5400
SYSCALL_NR_OPEN = 5
SYSCALL_NR_WRITE = 7
SYSCALL_NR_CLOSE = 8
SYSCALL_NR_MKDIR = 22
SYSCALL_NR_DLOPEN = 36
SYSCALL_NR_DLCLOSE = 37
SYSCALL_NR_DLSYM = 38
SYSCALL_NR_MODLOAD = 39
SYSCALL_NR_MODUNLOAD = 40
SYSCALL_NR_MODRELOAD = 41


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
    out = ROOT / "c-os/build/libqos_c_kernel_syscall_shared_module.so"
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
            str(ROOT / "c-os/kernel/irq/softirq.c"),
            str(ROOT / "c-os/kernel/timer/timer.c"),
            str(ROOT / "c-os/kernel/kthread/kthread.c"),
            str(ROOT / "c-os/kernel/net/napi.c"),
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


def _elf64_image(e_type: int) -> bytes:
    phoff = 64
    phentsize = 56
    phnum = 1
    image = bytearray(256)
    image[0:16] = bytes([0x7F, ord("E"), ord("L"), ord("F"), 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0])
    struct.pack_into("<HHIQQQIHHHHHH", image, 16, e_type, 0x3E, 1, 0x401000, phoff, 0, 0, 64, phentsize, phnum, 0, 0, 0)
    struct.pack_into("<IIQQQQQQ", image, phoff, 1, 5, 0, 0x400000, 0, 128, 128, 0x1000)
    return bytes(image)


def _write_file_via_syscalls(lib: ctypes.CDLL, path: bytes, data: bytes) -> None:
    p = ctypes.c_char_p(path)
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MKDIR, ctypes.cast(p, ctypes.c_void_p).value, 0, 0, 0) == 0
    fd = lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, ctypes.cast(p, ctypes.c_void_p).value, 0, 0, 0)
    assert fd >= 0
    payload = ctypes.create_string_buffer(data)
    assert (
        lib.qos_syscall_dispatch(
            SYSCALL_NR_WRITE,
            fd,
            ctypes.cast(payload, ctypes.c_void_p).value,
            len(data),
            0,
        )
        == len(data)
    )
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd, 0, 0, 0) == 0


def test_c_syscall_shared_lib_and_module_hot_reload_flow() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
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

    so_path = b"/tmp/libplug.so"
    _write_file_via_syscalls(lib, so_path, _elf64_image(3))

    handle1 = lib.qos_syscall_dispatch(SYSCALL_NR_DLOPEN, ctypes.cast(ctypes.c_char_p(so_path), ctypes.c_void_p).value, 0, 0, 0)
    assert handle1 > 0
    handle2 = lib.qos_syscall_dispatch(SYSCALL_NR_DLOPEN, ctypes.cast(ctypes.c_char_p(so_path), ctypes.c_void_p).value, 0, 0, 0)
    assert handle2 == handle1

    sym = ctypes.c_char_p(b"plugin_init")
    addr = lib.qos_syscall_dispatch(SYSCALL_NR_DLSYM, handle1, ctypes.cast(sym, ctypes.c_void_p).value, 0, 0)
    assert addr > 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_DLCLOSE, handle1, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_DLCLOSE, handle1, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_DLSYM, handle1, ctypes.cast(sym, ctypes.c_void_p).value, 0, 0) == -1

    mod_id = lib.qos_syscall_dispatch(SYSCALL_NR_MODLOAD, ctypes.cast(ctypes.c_char_p(so_path), ctypes.c_void_p).value, 0, 0, 0)
    assert mod_id > 0
    assert (
        lib.qos_syscall_dispatch(SYSCALL_NR_MODLOAD, ctypes.cast(ctypes.c_char_p(so_path), ctypes.c_void_p).value, 0, 0, 0)
        == mod_id
    )
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MODRELOAD, mod_id, 0, 0, 0) == 2
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MODRELOAD, mod_id, 0, 0, 0) == 3
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MODUNLOAD, mod_id, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MODRELOAD, mod_id, 0, 0, 0) == -1


def test_c_syscall_rejects_non_shared_images_for_dlopen_and_modload() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
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

    exec_path = b"/tmp/not_shared.elf"
    _write_file_via_syscalls(lib, exec_path, _elf64_image(2))

    assert (
        lib.qos_syscall_dispatch(SYSCALL_NR_DLOPEN, ctypes.cast(ctypes.c_char_p(exec_path), ctypes.c_void_p).value, 0, 0, 0)
        == -1
    )
    assert (
        lib.qos_syscall_dispatch(SYSCALL_NR_MODLOAD, ctypes.cast(ctypes.c_char_p(exec_path), ctypes.c_void_p).value, 0, 0, 0)
        == -1
    )
