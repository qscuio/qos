# Phase 4 Boot Info Fidelity Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Increase boot handoff fidelity by carrying real x86 E820 usable-memory metadata and AArch64 DTB pointer semantics into smoke-visible boot markers.

**Architecture:** Extend existing assembly boot artifacts only (no kernel/runtime crates yet). For x86, Stage1 captures E820 count plus first usable entry metadata into low-memory scratch and Stage2 writes/validates `boot_info` from it. For AArch64, preserve incoming `x0` as DTB pointer (with deterministic fallback) and store/validate `boot_info.dtb_addr`.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Expand smoke assertions for richer handoff semantics

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 assertions for first usable mmap markers**
- [x] **Step 2: Add AArch64 assertions for nonzero DTB handoff marker**
- [x] **Step 3: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Artifact Implementation

### Task 2: Implement x86 first-usable E820 entry propagation

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Capture first usable E820 entry fields in Stage1 scratch memory**
- [x] **Step 2: Populate `boot_info.mmap_entries[0]` from scratch data in Stage2**
- [x] **Step 3: Validate usable type/nonzero length in Stage2 before emitting success marker**
- [x] **Step 4: Emit x86 smoke markers for usable-first-entry semantics**

### Task 3: Implement AArch64 DTB pointer propagation marker

**Files:**
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Preserve incoming `x0` DTB pointer and store to `boot_info.dtb_addr`**
- [x] **Step 2: Add deterministic nonzero fallback if firmware DTB pointer is absent**
- [x] **Step 3: Validate nonzero `dtb_addr` in `kernel_main` and emit marker**

## Chunk 3: Verification

### Task 4: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase4-bootinfo-fidelity.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
