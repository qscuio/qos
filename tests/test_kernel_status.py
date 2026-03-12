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


class KernelStatus(ctypes.Structure):
    _fields_ = [
        ("init_state", ctypes.c_uint32),
        ("pmm_total_pages", ctypes.c_uint32),
        ("pmm_free_pages", ctypes.c_uint32),
        ("sched_count", ctypes.c_uint32),
        ("vfs_count", ctypes.c_uint32),
        ("net_queue_len", ctypes.c_uint32),
        ("drivers_count", ctypes.c_uint32),
        ("syscall_count", ctypes.c_uint32),
        ("proc_count", ctypes.c_uint32),
    ]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_kernel_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_kernel_status.so"
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


def test_c_kernel_status_snapshot_tracks_subsystems() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_kernel_status_snapshot.argtypes = [ctypes.POINTER(KernelStatus)]
    lib.qos_kernel_status_snapshot.restype = ctypes.c_int
    lib.qos_vfs_create.argtypes = [ctypes.c_char_p]
    lib.qos_vfs_create.restype = ctypes.c_int
    lib.qos_drivers_register.argtypes = [ctypes.c_char_p]
    lib.qos_drivers_register.restype = ctypes.c_int
    lib.qos_proc_create.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.qos_proc_create.restype = ctypes.c_int
    lib.qos_sched_add_task.argtypes = [ctypes.c_uint32]
    lib.qos_sched_add_task.restype = ctypes.c_int
    lib.qos_syscall_register.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_int64]
    lib.qos_syscall_register.restype = ctypes.c_int
    lib.qos_net_enqueue_packet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_net_enqueue_packet.restype = ctypes.c_int

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0

    baseline = KernelStatus()
    assert lib.qos_kernel_status_snapshot(ctypes.byref(baseline)) == 0
    assert baseline.syscall_count >= 9

    assert lib.qos_vfs_create(b"/init") == 0
    assert lib.qos_drivers_register(b"uart") == 0
    assert lib.qos_proc_create(1, 0) == 0
    assert lib.qos_sched_add_task(1) == 0
    assert lib.qos_syscall_register(90, 1, 42) == 0
    pkt = (ctypes.c_ubyte * 2)(7, 9)
    assert lib.qos_net_enqueue_packet(pkt, 2) == 0

    status = KernelStatus()
    assert lib.qos_kernel_status_snapshot(ctypes.byref(status)) == 0
    assert status.pmm_total_pages > 0
    assert status.pmm_free_pages > 0
    assert status.vfs_count == 1
    assert status.drivers_count == 1
    assert status.proc_count == 1
    assert status.sched_count == 1
    assert status.syscall_count == baseline.syscall_count + 1
    assert status.net_queue_len == 1
