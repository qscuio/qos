from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_mm_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_mm_vmm_isolation.so"
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


def test_c_vmm_supports_per_asid_isolation() -> None:
    lib = ctypes.CDLL(str(_build_mm_lib()))
    lib.qos_vmm_reset.argtypes = []
    lib.qos_vmm_reset.restype = None
    lib.qos_vmm_set_asid.argtypes = [ctypes.c_uint32]
    lib.qos_vmm_set_asid.restype = None
    lib.qos_vmm_get_asid.argtypes = []
    lib.qos_vmm_get_asid.restype = ctypes.c_uint32
    lib.qos_vmm_map.argtypes = [ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_vmm_map.restype = ctypes.c_int
    lib.qos_vmm_unmap.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_unmap.restype = ctypes.c_int
    lib.qos_vmm_translate.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_translate.restype = ctypes.c_uint64
    lib.qos_vmm_map_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_vmm_map_as.restype = ctypes.c_int
    lib.qos_vmm_unmap_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_vmm_unmap_as.restype = ctypes.c_int
    lib.qos_vmm_translate_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_vmm_translate_as.restype = ctypes.c_uint64
    lib.qos_vmm_flags_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_vmm_flags_as.restype = ctypes.c_uint32

    va = 0x4000
    pa_as1 = 0xA000
    pa_as2 = 0xB000

    lib.qos_vmm_reset()
    assert lib.qos_vmm_get_asid() == 0

    assert lib.qos_vmm_map_as(1, va, pa_as1, 0x3) == 0
    assert lib.qos_vmm_map_as(2, va, pa_as2, 0x5) == 0
    assert lib.qos_vmm_translate_as(1, va) == pa_as1
    assert lib.qos_vmm_translate_as(2, va) == pa_as2
    assert lib.qos_vmm_flags_as(1, va) == 0x3
    assert lib.qos_vmm_flags_as(2, va) == 0x5

    lib.qos_vmm_set_asid(1)
    assert lib.qos_vmm_get_asid() == 1
    assert lib.qos_vmm_translate(va) == pa_as1

    lib.qos_vmm_set_asid(2)
    assert lib.qos_vmm_get_asid() == 2
    assert lib.qos_vmm_translate(va) == pa_as2

    assert lib.qos_vmm_unmap_as(1, va) == 0
    assert lib.qos_vmm_translate_as(1, va) == 0
    assert lib.qos_vmm_translate_as(2, va) == pa_as2
    assert lib.qos_vmm_unmap_as(2, va) == 0
    assert lib.qos_vmm_translate_as(2, va) == 0


def test_c_vmm_rejects_invalid_asid_or_alignment() -> None:
    lib = ctypes.CDLL(str(_build_mm_lib()))
    lib.qos_vmm_reset.argtypes = []
    lib.qos_vmm_reset.restype = None
    lib.qos_vmm_map_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32]
    lib.qos_vmm_map_as.restype = ctypes.c_int
    lib.qos_vmm_unmap_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_vmm_unmap_as.restype = ctypes.c_int
    lib.qos_vmm_translate_as.argtypes = [ctypes.c_uint32, ctypes.c_uint64]
    lib.qos_vmm_translate_as.restype = ctypes.c_uint64

    lib.qos_vmm_reset()
    assert lib.qos_vmm_map_as(64, 0x4000, 0x8000, 1) == -1
    assert lib.qos_vmm_map_as(1, 0x4001, 0x8000, 1) == -1
    assert lib.qos_vmm_map_as(1, 0x4000, 0x8001, 1) == -1
    assert lib.qos_vmm_unmap_as(64, 0x4000) == -1
    assert lib.qos_vmm_translate_as(64, 0x4000) == 0
