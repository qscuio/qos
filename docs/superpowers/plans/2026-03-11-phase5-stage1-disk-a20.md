# Phase 5 x86 Stage1 Disk + A20 Fidelity Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve x86 Stage1 realism by enabling A20 and preferring BIOS LBA extensions for Stage2 loading with CHS fallback, surfaced through deterministic smoke markers.

**Architecture:** Keep the two-stage boot flow unchanged while upgrading Stage1 behavior. Stage1 now executes A20 enable logic, probes Int13 extensions, attempts DAP/LBA Stage2 read first, and falls back to CHS reads. Stage2 validates/prints the selected load path (`disk_load`) and includes A20 marker in serial output.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Expand x86 smoke contract for disk path + A20 marker

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 marker requirement for `a20=enabled`**
- [x] **Step 2: Add x86 regex assertion for `disk_load=(lba_ext|chs)`**
- [x] **Step 3: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Implement Stage1 A20 enable + LBA-first load path

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Add Stage1 `enable_a20` routine (port 0x92 + BIOS fallback)**
- [x] **Step 2: Implement Int13 extension probe and DAP/LBA Stage2 read path**
- [x] **Step 3: Preserve CHS fallback path and record selected method in low-memory scratch**
- [x] **Step 4: Update Stage2 serial markers to print `disk_load` and `a20` status markers**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase5-stage1-disk-a20.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
