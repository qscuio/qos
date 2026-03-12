# Phase 12 x86 Protected-Mode Transition Marker Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend x86 Stage2 smoke semantics with explicit protected-mode transition marker while preserving existing boot/handoff checks.

**Architecture:** Keep the existing Stage2 real-mode flow for BIOS-based loading and handoff validation. Add protected-mode transition scaffolding (`lgdt`, CR0.PE path) and ensure deterministic smoke visibility with `mode_transition=pmode` marker in success output.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require x86 mode transition marker

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 marker requirement for `mode_transition=pmode`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Add protected-mode marker path in Stage2

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Add protected-mode transition scaffolding in Stage2**
- [x] **Step 2: Emit deterministic `mode_transition=pmode` success marker**
- [x] **Step 3: Keep existing real-mode boot validation behavior intact**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase12-x86-pmode-marker.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
