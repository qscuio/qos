from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

VM_READ = 1 << 0
VM_WRITE = 1 << 1
VM_EXEC = 1 << 2


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_mm_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_mm_vmm.so"
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
            str(ROOT / "c-os/kernel/mm/mm.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_vmm_map_translate_unmap_round_trip() -> None:
    lib = ctypes.CDLL(str(_build_mm_lib()))
    lib.qos_vmm_reset.argtypes = []
    lib.qos_vmm_reset.restype = None
    lib.qos_vmm_map.argtypes = [ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_vmm_map.restype = ctypes.c_int
    lib.qos_vmm_translate.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_translate.restype = ctypes.c_uint64
    lib.qos_vmm_flags.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_flags.restype = ctypes.c_uint32
    lib.qos_vmm_unmap.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_unmap.restype = ctypes.c_int

    lib.qos_vmm_reset()
    assert lib.qos_vmm_map(0x4000, 0x200000, VM_READ | VM_WRITE) == 0
    assert lib.qos_vmm_translate(0x4000) == 0x200000
    assert lib.qos_vmm_translate(0x4123) == 0x200123
    assert lib.qos_vmm_flags(0x4000) == (VM_READ | VM_WRITE)
    assert lib.qos_vmm_unmap(0x4000) == 0
    assert lib.qos_vmm_translate(0x4000) == 0


def test_c_vmm_rejects_unaligned_addresses() -> None:
    lib = ctypes.CDLL(str(_build_mm_lib()))
    lib.qos_vmm_reset.argtypes = []
    lib.qos_vmm_reset.restype = None
    lib.qos_vmm_map.argtypes = [ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_vmm_map.restype = ctypes.c_int
    lib.qos_vmm_unmap.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_unmap.restype = ctypes.c_int

    lib.qos_vmm_reset()
    assert lib.qos_vmm_map(0x4001, 0x200000, VM_READ) != 0
    assert lib.qos_vmm_map(0x4000, 0x200001, VM_READ) != 0
    assert lib.qos_vmm_unmap(0x4001) != 0


def test_c_vmm_remap_updates_translation() -> None:
    lib = ctypes.CDLL(str(_build_mm_lib()))
    lib.qos_vmm_reset.argtypes = []
    lib.qos_vmm_reset.restype = None
    lib.qos_vmm_map.argtypes = [ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_vmm_map.restype = ctypes.c_int
    lib.qos_vmm_translate.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_translate.restype = ctypes.c_uint64
    lib.qos_vmm_flags.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_flags.restype = ctypes.c_uint32

    lib.qos_vmm_reset()
    assert lib.qos_vmm_map(0x8000, 0x300000, VM_READ) == 0
    assert lib.qos_vmm_map(0x8000, 0x500000, VM_EXEC) == 0
    assert lib.qos_vmm_translate(0x8000) == 0x500000
    assert lib.qos_vmm_flags(0x8000) == VM_EXEC


def test_c_vmm_runtime_mapping_snapshot_by_asid() -> None:
    lib = ctypes.CDLL(str(_build_mm_lib()))
    lib.qos_vmm_reset.argtypes = []
    lib.qos_vmm_reset.restype = None
    lib.qos_vmm_set_asid.argtypes = [ctypes.c_uint32]
    lib.qos_vmm_set_asid.restype = None
    lib.qos_vmm_map_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_vmm_map_as.restype = ctypes.c_int
    lib.qos_vmm_mapping_count_as.argtypes = [ctypes.c_uint32]
    lib.qos_vmm_mapping_count_as.restype = ctypes.c_uint32
    lib.qos_vmm_mapping_get_as.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_uint64),
        ctypes.POINTER(ctypes.c_uint64),
        ctypes.POINTER(ctypes.c_uint32),
    ]
    lib.qos_vmm_mapping_get_as.restype = ctypes.c_int

    lib.qos_vmm_reset()
    lib.qos_vmm_set_asid(7)
    assert lib.qos_vmm_map_as(7, 0x4000, 0x200000, VM_READ | VM_WRITE) == 0
    assert lib.qos_vmm_map_as(7, 0x8000, 0x210000, VM_READ) == 0
    assert lib.qos_vmm_mapping_count_as(7) == 2

    va = ctypes.c_uint64(0)
    pa = ctypes.c_uint64(0)
    flags = ctypes.c_uint32(0)
    assert lib.qos_vmm_mapping_get_as(7, 0, ctypes.byref(va), ctypes.byref(pa), ctypes.byref(flags)) == 0
    assert va.value == 0x4000
    assert pa.value == 0x200000
    assert flags.value == (VM_READ | VM_WRITE)

    assert lib.qos_vmm_mapping_get_as(7, 1, ctypes.byref(va), ctypes.byref(pa), ctypes.byref(flags)) == 0
    assert va.value == 0x8000
    assert pa.value == 0x210000
    assert flags.value == VM_READ
