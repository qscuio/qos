# Phase 17 x86 Kernel Magic Marker Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expose explicit x86 kernel ELF magic validation result in smoke markers.

**Architecture:** Reuse the existing Stage2 ELF header validation path and add a dedicated marker (`kernel_magic=ELF`) to make the invariant explicit in serial output. Preserve current validation logic and all prior markers.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require kernel magic marker for x86

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 marker requirement for `kernel_magic=ELF`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Emit explicit kernel magic marker

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Add `kernel_magic=ELF` to x86 success output**
- [x] **Step 2: Preserve existing ELF magic validation logic**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase17-x86-kernel-magic-marker.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
