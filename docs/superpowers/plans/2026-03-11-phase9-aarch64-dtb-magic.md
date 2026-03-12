# Phase 9 AArch64 DTB Magic Validation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Strengthen AArch64 boot handoff semantics by validating the DTB header magic, not just nonzero DTB pointer.

**Architecture:** At `_start`, prefer firmware-provided `x0` DTB pointer when it contains valid FDT magic; otherwise fall back to a minimal built-in fake DTB blob to keep smoke deterministic. In `kernel_main`, verify `boot_info.dtb_addr` is nonzero and points to a valid DTB magic before emitting success marker.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require DTB magic marker in AArch64 smoke output

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add AArch64 marker requirement for `dtb_magic=ok`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Implement DTB magic validation + fallback DTB blob

**Files:**
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Validate firmware DTB magic at `_start` when `x0 != 0`**
- [x] **Step 2: Add deterministic fallback pointer to built-in fake DTB**
- [x] **Step 3: Verify DTB magic in `kernel_main` before success output**
- [x] **Step 4: Extend success marker with `dtb_magic=ok`**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase9-aarch64-dtb-magic.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
