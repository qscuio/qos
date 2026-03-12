# Phase 7 x86 Stage2 Initramfs Header Load Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make x86 Stage2 perform a real BIOS disk read of initramfs header data at LBA 4224 and surface success with deterministic smoke marker.

**Architecture:** Reuse the existing Stage2 real-mode Int13 extension pattern used for kernel header loading. Seed a minimal CPIO newc header sector in the image (`070701`), validate it after read, and populate nonzero `boot_info.initramfs_addr/initramfs_size` fields. Keep ACPI/DTB x86 placeholders unchanged.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require initramfs-load marker

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Replace x86 `initramfs_addr=0/initramfs_size=0` marker expectations with `initramfs_load=cpio`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Implement Stage2 initramfs header read/verify

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Add Stage2 Int13 extension loader for LBA 4224 into `0xB000`**
- [x] **Step 2: Validate CPIO newc magic prefix (`070701`)**
- [x] **Step 3: Populate `boot_info.initramfs_addr/initramfs_size` with nonzero values**
- [x] **Step 4: Update x86 success marker to include `initramfs_load=cpio`**
- [x] **Step 5: Seed disk image at LBA 4224 with a minimal CPIO header sector**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase7-x86-initramfs-header.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
