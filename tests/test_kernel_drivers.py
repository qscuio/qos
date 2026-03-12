from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class NicDesc(ctypes.Structure):
    _fields_ = [
        ("mmio_base", ctypes.c_uint64),
        ("irq", ctypes.c_uint32),
        ("tx_desc_count", ctypes.c_uint16),
        ("rx_desc_count", ctypes.c_uint16),
        ("tx_head", ctypes.c_uint16),
        ("tx_tail", ctypes.c_uint16),
        ("rx_head", ctypes.c_uint16),
        ("rx_tail", ctypes.c_uint16),
    ]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_drivers_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_drivers.so"
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
            str(ROOT / "c-os/kernel/drivers/drivers.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_driver_register_and_lookup() -> None:
    lib = ctypes.CDLL(str(_build_drivers_lib()))
    lib.qos_drivers_reset.argtypes = []
    lib.qos_drivers_reset.restype = None
    lib.qos_drivers_register.argtypes = [ctypes.c_char_p]
    lib.qos_drivers_register.restype = ctypes.c_int
    lib.qos_drivers_loaded.argtypes = [ctypes.c_char_p]
    lib.qos_drivers_loaded.restype = ctypes.c_int
    lib.qos_drivers_count.argtypes = []
    lib.qos_drivers_count.restype = ctypes.c_uint32

    lib.qos_drivers_reset()
    assert lib.qos_drivers_register(b"uart") == 0
    assert lib.qos_drivers_register(b"e1000") == 0
    assert lib.qos_drivers_loaded(b"uart") == 1
    assert lib.qos_drivers_loaded(b"e1000") == 1
    assert lib.qos_drivers_loaded(b"virtio-net") == 0
    assert lib.qos_drivers_count() == 2


def test_c_driver_rejects_duplicate_and_invalid_names() -> None:
    lib = ctypes.CDLL(str(_build_drivers_lib()))
    lib.qos_drivers_reset.argtypes = []
    lib.qos_drivers_reset.restype = None
    lib.qos_drivers_register.argtypes = [ctypes.c_char_p]
    lib.qos_drivers_register.restype = ctypes.c_int

    lib.qos_drivers_reset()
    assert lib.qos_drivers_register(b"uart") == 0
    assert lib.qos_drivers_register(b"uart") != 0
    assert lib.qos_drivers_register(b"") != 0


def test_c_driver_nic_metadata_and_ring_bookkeeping() -> None:
    lib = ctypes.CDLL(str(_build_drivers_lib()))
    lib.qos_drivers_reset.argtypes = []
    lib.qos_drivers_reset.restype = None
    lib.qos_drivers_register_nic.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint64,
        ctypes.c_uint32,
        ctypes.c_uint16,
        ctypes.c_uint16,
    ]
    lib.qos_drivers_register_nic.restype = ctypes.c_int
    lib.qos_drivers_get_nic.argtypes = [ctypes.c_char_p, ctypes.POINTER(NicDesc)]
    lib.qos_drivers_get_nic.restype = ctypes.c_int
    lib.qos_drivers_nic_advance_tx.argtypes = [ctypes.c_char_p, ctypes.c_uint16]
    lib.qos_drivers_nic_advance_tx.restype = ctypes.c_int
    lib.qos_drivers_nic_consume_rx.argtypes = [ctypes.c_char_p, ctypes.c_uint16]
    lib.qos_drivers_nic_consume_rx.restype = ctypes.c_int

    lib.qos_drivers_reset()
    assert lib.qos_drivers_register_nic(b"e1000", 0xFEE00000, 11, 16, 16) == 0
    assert lib.qos_drivers_register_nic(b"virtio-net", 0x0A000000, 1, 16, 16) == 0
    assert lib.qos_drivers_register_nic(b"e1000", 0xFEE00000, 11, 16, 16) == -1

    nic = NicDesc()
    assert lib.qos_drivers_get_nic(b"e1000", ctypes.byref(nic)) == 0
    assert nic.mmio_base == 0xFEE00000
    assert nic.irq == 11
    assert nic.tx_desc_count == 16
    assert nic.rx_desc_count == 16
    assert nic.tx_head == 0
    assert nic.tx_tail == 0
    assert nic.rx_head == 0
    assert nic.rx_tail == 0

    assert lib.qos_drivers_nic_advance_tx(b"e1000", 3) == 0
    assert lib.qos_drivers_nic_consume_rx(b"e1000", 2) == 0
    assert lib.qos_drivers_get_nic(b"e1000", ctypes.byref(nic)) == 0
    assert nic.tx_tail == 3
    assert nic.rx_head == 2

    assert lib.qos_drivers_nic_advance_tx(b"e1000", 20) == 0  # wraps mod 16
    assert lib.qos_drivers_get_nic(b"e1000", ctypes.byref(nic)) == 0
    assert nic.tx_tail == (3 + 20) % 16
