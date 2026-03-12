# Phase 6 x86 Stage2 Kernel ELF Header Load Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make x86 Stage2 perform a real BIOS disk read of a kernel ELF header at LBA 128 and expose success via deterministic smoke marker.

**Architecture:** Keep current real-mode Stage2 for smoke while adding BIOS Int13 extension-based LBA read logic. Seed disk image with a minimal ELF header sector at LBA 128, then validate `0x7F 'E' 'L' 'F'` signature in Stage2 before printing success markers. Preserve prior handoff checks and marker stability.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require kernel ELF load marker

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 marker requirement for `kernel_load=elf`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Implement Stage2 kernel header read/verify

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Stage1 stores boot drive byte in low-memory scratch for Stage2**
- [x] **Step 2: Stage2 reads LBA 128 via Int13 extensions into scratch buffer**
- [x] **Step 3: Stage2 validates ELF magic and fails handoff on mismatch**
- [x] **Step 4: Add `kernel_load=elf` marker to x86 success output**
- [x] **Step 5: Seed disk image at LBA 128 with a minimal ELF-header sector**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase6-x86-kernel-elf-header.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
