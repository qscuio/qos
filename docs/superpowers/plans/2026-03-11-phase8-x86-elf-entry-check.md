# Phase 8 x86 Stage2 ELF Entry Check Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ensure x86 Stage2 validates more than ELF magic by asserting the loaded kernel header has a nonzero entry field and surfacing that as a smoke marker.

**Architecture:** Reuse the existing LBA-128 header load path and add a simple nonzero check over `e_entry` bytes in the loaded header buffer. Keep smoke deterministic by seeding the test image with a minimal nonzero entry byte at the expected offset.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require ELF entry marker in x86 smoke output

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 marker requirement for `kernel_entry_nonzero=1`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Implement ELF entry-field validation in Stage2

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Validate nonzero `e_entry` bytes after ELF magic checks**
- [x] **Step 2: Add `kernel_entry_nonzero=1` to x86 success marker**
- [x] **Step 3: Seed the image header sector with nonzero entry byte at offset `0x18`**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase8-x86-elf-entry-check.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
