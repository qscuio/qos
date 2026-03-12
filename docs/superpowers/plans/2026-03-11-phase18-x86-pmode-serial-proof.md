# Phase 18 x86 Protected-Mode Serial Proof Marker Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ensure smoke logs prove the dedicated protected-mode print path executed (not just inline marker text).

**Architecture:** Reuse existing Stage2 protected-mode transition path and add an explicit marker (`pmode_serial=32bit`) in the protected-mode-only serial line. Keep existing real-mode marker line unchanged.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require protected-mode serial proof marker

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 marker requirement for `pmode_serial=32bit`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Emit proof marker from protected-mode line

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Add `pmode_serial=32bit` to protected-mode serial message**
- [x] **Step 2: Preserve current protected-mode transition and print behavior**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase18-x86-pmode-serial-proof.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
