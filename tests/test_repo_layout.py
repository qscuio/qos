from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


EXPECTED_DIRS = [
    "c-os",
    "rust-os",
    "linux-1.0.0",
    "linux-lab",
    "xv6",
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

LINUX1_OWNED_PATHS = [
    "linux-1.0.0/userspace/src/init.S",
    "linux-1.0.0/userspace/src/sh.S",
    "linux-1.0.0/manifests/linux1-sources.lock",
    "linux-1.0.0/patches/lilo",
]

REMOVED_TOP_LEVEL_PATHS = [
    "linux1-userspace",
]


def test_repository_layout_matches_spec_scaffold() -> None:
    missing = [path for path in EXPECTED_DIRS if not (ROOT / path).is_dir()]
    assert missing == [], f"missing directories: {missing}"


def test_linux1_owned_paths_live_under_linux_1_0_0() -> None:
    missing = [path for path in LINUX1_OWNED_PATHS if not (ROOT / path).exists()]
    assert missing == [], f"missing linux1-owned paths: {missing}"


def test_old_top_level_linux1_owned_paths_are_gone() -> None:
    present = [path for path in REMOVED_TOP_LEVEL_PATHS if (ROOT / path).exists()]
    assert present == [], f"stale top-level linux1-owned paths remain: {present}"


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
