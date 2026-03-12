from __future__ import annotations

from pathlib import Path

from tools.boot_info_guard import validate_boot_info_contract


def _has(code: str, issues: list[str]) -> bool:
    return any(issue.startswith(code) for issue in issues)


def test_flags_field_count_mismatch() -> None:
    c_text = """
typedef struct {
    uint64_t magic;
    uint32_t mmap_entry_count;
} boot_info_t;
"""
    r_text = """
#[repr(C)]
pub struct BootInfo {
    pub magic: u64,
}
"""
    issues = validate_boot_info_contract(c_text, r_text)
    assert _has("B001", issues)


def test_flags_field_name_mismatch() -> None:
    c_text = """
typedef struct {
    uint64_t magic;
    uint32_t mmap_entry_count;
} boot_info_t;
"""
    r_text = """
#[repr(C)]
pub struct BootInfo {
    pub magic: u64,
    pub mmap_count: u32,
}
"""
    issues = validate_boot_info_contract(c_text, r_text)
    assert _has("B002", issues)


def test_real_boot_info_contract_is_consistent() -> None:
    root = Path(__file__).resolve().parents[1]
    c_text = (root / "c-os/boot/boot_info.h").read_text(encoding="utf-8")
    r_text = (root / "rust-os/boot/boot_info.rs").read_text(encoding="utf-8")

    issues = validate_boot_info_contract(c_text, r_text)
    assert issues == []
