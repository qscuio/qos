# Phase 16 AArch64 Mmap Length Marker Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make AArch64 smoke output explicitly report that the first mmap entry length is nonzero after boot_info validation.

**Architecture:** Reuse existing AArch64 mmap stub initialization and validation, and expose the invariant through a dedicated serial marker (`mmap_len_nonzero=1`) for deterministic smoke assertions.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require mmap-length marker for AArch64

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add AArch64 marker requirement for `mmap_len_nonzero=1`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Emit explicit mmap-length marker

**Files:**
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Add `mmap_len_nonzero=1` to AArch64 success marker**
- [x] **Step 2: Preserve existing mmap nonzero-length validation behavior**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase16-aarch64-mmap-len-marker.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
