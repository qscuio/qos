# Phase 14 AArch64 Initramfs Size Marker Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expose validated nonzero AArch64 initramfs size semantics through an explicit smoke marker.

**Architecture:** Reuse existing AArch64 boot_info initramfs population/validation path and add a dedicated marker to make the invariant visible in serial smoke output.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require AArch64 initramfs-size marker

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add AArch64 marker requirement for `initramfs_size_nonzero=1`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Emit explicit AArch64 initramfs-size marker

**Files:**
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Add `initramfs_size_nonzero=1` marker to AArch64 success output**
- [x] **Step 2: Preserve existing initramfs nonzero validation behavior**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase14-aarch64-initramfs-size-marker.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
