# Phase 10 AArch64 Mmap Stub Handoff Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Increase AArch64 `boot_info` fidelity by populating and validating a minimal usable mmap entry in addition to DTB pointer semantics.

**Architecture:** Keep current deterministic AArch64 smoke boot path, but set `mmap_entry_count=1` and initialize `mmap_entries[0]` with a simple usable range. Validate these fields in `kernel_main` and expose deterministic markers (`mmap_source=dtb_stub`, `mmap_count=1`) in serial output.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require AArch64 mmap markers

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add AArch64 marker requirements for `mmap_source=dtb_stub` and `mmap_count=1`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Populate and validate minimal mmap in AArch64 boot_info

**Files:**
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Populate `boot_info.mmap_entry_count=1` at `_start`**
- [x] **Step 2: Initialize first mmap entry with nonzero length and usable type**
- [x] **Step 3: Validate mmap fields in `kernel_main` before success output**
- [x] **Step 4: Emit AArch64 mmap markers in success message**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase10-aarch64-mmap-stub.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
