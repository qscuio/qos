from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


EXPECTED_DIRS = [
    "c-os/boot/x86_64",
    "c-os/boot/aarch64",
    "c-os/kernel/arch/x86_64",
    "c-os/kernel/arch/aarch64",
    "c-os/kernel/mm",
    "c-os/kernel/sched",
    "c-os/kernel/fs",
    "c-os/kernel/net",
    "c-os/kernel/drivers",
    "c-os/libc",
    "c-os/userspace",
    "rust-os/boot",
    "rust-os/kernel",
    "rust-os/libc",
    "rust-os/userspace",
]


def test_repository_layout_matches_spec_scaffold() -> None:
    missing = [path for path in EXPECTED_DIRS if not (ROOT / path).is_dir()]
    assert missing == [], f"missing directories: {missing}"


def test_boot_info_contract_files_exist_and_include_required_fields() -> None:
    c_header = ROOT / "c-os/boot/boot_info.h"
    rust_mod = ROOT / "rust-os/boot/boot_info.rs"

    assert c_header.is_file(), f"missing file: {c_header}"
    assert rust_mod.is_file(), f"missing file: {rust_mod}"

    c_text = c_header.read_text(encoding="utf-8")
    r_text = rust_mod.read_text(encoding="utf-8")

    required_fields = [
        "magic",
        "mmap_entry_count",
        "mmap_entries",
        "initramfs_addr",
        "initramfs_size",
        "acpi_rsdp_addr",
        "dtb_addr",
    ]

    for field in required_fields:
        assert field in c_text, f"C boot_info is missing field {field!r}"
        assert field in r_text, f"Rust boot_info is missing field {field!r}"


def test_rust_workspace_manifest_declares_core_members() -> None:
    manifest = ROOT / "rust-os/Cargo.toml"
    assert manifest.is_file(), f"missing file: {manifest}"

    text = manifest.read_text(encoding="utf-8")
    assert "[workspace]" in text
    for member in ("boot", "kernel", "libc", "userspace"):
        assert f'"{member}"' in text
