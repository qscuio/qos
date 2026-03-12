# Phase 3 Boot Chain + Handoff Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move from single-entry placeholders to explicit boot-stage behavior and handoff markers in real QEMU serial smoke logs.

**Architecture:** Implement a minimal x86 two-stage path (Stage1 sector loader + Stage2 serial payload) and an AArch64 `_start -> kernel_main` handoff path that passes a `boot_info` pointer. Preserve fast smoke execution by using timeout-bounded QEMU runs and deterministic serial assertions.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Add failing smoke assertions for stage/handoff semantics

**Files:**
- Create: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Write failing test for `stage=stage2` on x86 targets**
- [x] **Step 2: Write failing test for `handoff=boot_info` on AArch64 targets**
- [x] **Step 3: Run test and confirm failure before implementation**

## Chunk 2: Boot Implementation

### Task 2: Implement x86 Stage1 -> Stage2 chain

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Build Stage2 real-mode serial payload binary**
- [x] **Step 2: Build Stage1 boot sector that reads Stage2 from disk and jumps to 0x9000**
- [x] **Step 3: Pack Stage1+Stage2 into disk image layout (LBA0/LBA1+)**

### Task 3: Implement AArch64 boot handoff marker

**Files:**
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Add `_start` entry that sets `x0` to boot_info pointer**
- [x] **Step 2: Call `kernel_main` and validate pointer handoff in serial output**

## Chunk 3: Integration + Verification

### Task 4: End-to-end verification

**Files:**
- Modify: `c-os/Makefile`
- Modify: `rust-os/Makefile`
- Modify: `docs/superpowers/plans/2026-03-11-phase3-boot-handoff.md`

- [x] **Step 1: Ensure implementation Makefiles rebuild boot artifacts for smoke runs**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all` and verify all 4 matrix entries pass**
