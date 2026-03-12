# Phase 2 QEMU Serial Smoke Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace synthetic smoke logs with real QEMU serial boot markers for all 4 implementation/architecture targets.

**Architecture:** Generate minimal bootable artifacts per target (x86_64 boot sector image and AArch64 kernel ELF) that print deterministic serial markers. Update smoke harness to execute QEMU with timeouts, capture serial output, and assert markers in logs.

**Tech Stack:** GNU Make, Bash, GNU binutils cross toolchains, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Add failing test for real serial markers

**Files:**
- Create: `tests/test_qemu_serial_smoke.py`

- [x] **Step 1: Write failing test requiring per-target serial marker in smoke logs**
- [x] **Step 2: Run test and confirm failure against synthetic smoke script**

## Chunk 2: Implementation

### Task 2: Build real boot artifacts and run QEMU smoke

**Files:**
- Create: `scripts/build_x86_boot_img.sh`
- Create: `scripts/build_aarch64_kernel.sh`
- Modify: `c-os/Makefile`
- Modify: `rust-os/Makefile`
- Modify: `scripts/smoke_target.sh`
- Modify: `qemu/x86_64.sh`
- Modify: `qemu/aarch64.sh`

- [x] **Step 1: Add boot artifact generator scripts**
- [x] **Step 2: Wire implementation Makefiles to build architecture-specific boot artifacts**
- [x] **Step 3: Replace synthetic smoke with timeout-bounded QEMU execution + serial log assertions**
- [x] **Step 4: Ensure QEMU wrappers accept artifact paths needed by smoke harness**

## Chunk 3: Verification

### Task 3: Verify full matrix

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase2-qemu-serial-smoke.md` (checkbox updates)

- [x] **Step 1: Run `pytest -q tests/test_qemu_serial_smoke.py`**
- [x] **Step 2: Run full `pytest -q`**
- [x] **Step 3: Run `make test-all` and verify all 4 matrix entries pass with real serial markers**
