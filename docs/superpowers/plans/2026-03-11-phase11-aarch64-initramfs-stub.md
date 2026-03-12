# Phase 11 AArch64 Initramfs Stub Handoff Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add deterministic AArch64 initramfs handoff semantics and smoke visibility while keeping boot path lightweight.

**Architecture:** Generate a small per-target aarch64 initramfs artifact and pass it through the smoke harness to QEMU `-initrd`. In boot code, populate nonzero `boot_info` initramfs fields and validate them before success output, then emit `initramfs_source=stub` marker for deterministic smoke assertions.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require AArch64 initramfs marker

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add AArch64 marker requirement for `initramfs_source=stub`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Build + Boot Implementation

### Task 2: Wire initrd artifact through build/smoke path and boot markers

**Files:**
- Create: `scripts/build_aarch64_initramfs.sh`
- Modify: `c-os/Makefile`
- Modify: `rust-os/Makefile`
- Modify: `scripts/smoke_target.sh`
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Add script to build minimal AArch64 initramfs artifact**
- [x] **Step 2: Ensure AArch64 build target emits both kernel ELF and initramfs artifact**
- [x] **Step 3: Extend smoke harness for optional initrd argument**
- [x] **Step 4: Populate/validate nonzero AArch64 initramfs fields in boot_info**
- [x] **Step 5: Emit `initramfs_source=stub` in AArch64 success marker**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase11-aarch64-initramfs-stub.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
