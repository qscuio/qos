from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QOS_BOOT_MAGIC = 0x514F53424F4F5400


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


def _build_kernel_libc_bridge() -> Path:
    out = ROOT / "c-os/build/libqos_c_libc_kernel_bridge.so"
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


def test_c_libc_socket_calls_route_to_kernel_when_available() -> None:
    lib = ctypes.CDLL(str(_build_kernel_libc_bridge()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_socket.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib.qos_socket.restype = ctypes.c_int
    lib.qos_bind.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_bind.restype = ctypes.c_int
    lib.qos_sendto.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_sendto.restype = ctypes.c_ssize_t
    lib.qos_recvfrom.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    lib.qos_recvfrom.restype = ctypes.c_ssize_t
    lib.qos_close.argtypes = [ctypes.c_int]
    lib.qos_close.restype = ctypes.c_int

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0

    recv_sock = lib.qos_socket(2, 2, 0)
    send_sock = lib.qos_socket(2, 2, 0)
    assert recv_sock == 0
    assert send_sock == 1

    recv_addr = (ctypes.c_ubyte * 16)()
    recv_addr[2] = 0x0F
    recv_addr[3] = 0xA1  # 4001
    recv_addr[4] = 0x00
    recv_addr[5] = 0x00
    recv_addr[6] = 0x00
    recv_addr[7] = 0x00  # INADDR_ANY should be accepted on bind
    assert lib.qos_bind(recv_sock, ctypes.cast(recv_addr, ctypes.c_void_p), 16) == 0

    payload = (ctypes.c_ubyte * 3)(ord("q"), ord("o"), ord("s"))
    dst_addr = (ctypes.c_ubyte * 16)()
    dst_addr[2] = 0x0F
    dst_addr[3] = 0xA1
    dst_addr[4] = 0x0A
    dst_addr[5] = 0x00
    dst_addr[6] = 0x02
    dst_addr[7] = 0x15
    sent = lib.qos_sendto(send_sock, ctypes.cast(payload, ctypes.c_void_p), 3, 0, ctypes.cast(dst_addr, ctypes.c_void_p), 16)
    assert sent == 3

    out = (ctypes.c_ubyte * 16)()
    src_addr = (ctypes.c_ubyte * 16)()
    src_len = ctypes.c_uint32(0)
    got = lib.qos_recvfrom(
        recv_sock,
        ctypes.cast(out, ctypes.c_void_p),
        16,
        0,
        ctypes.cast(src_addr, ctypes.c_void_p),
        ctypes.byref(src_len),
    )
    assert got == 3
    assert bytes(out[:3]) == b"qos"
    assert src_len.value == 16

    assert lib.qos_close(send_sock) == 0
    assert lib.qos_close(recv_sock) == 0
