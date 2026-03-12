# Phase 13 x86 ELF Program-Header Presence Check Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve x86 Stage2 ELF validation fidelity by requiring nonzero program-header metadata (`e_phoff`, `e_phnum`) before success.

**Architecture:** Extend the current LBA-128 ELF header validation path with additional checks for program-header presence. Keep deterministic smoke behavior by seeding the synthetic ELF header sector with nonzero values for these fields and exposing success with a dedicated marker.

**Tech Stack:** GNU binutils cross toolchains, Bash, GNU Make, QEMU, pytest

---

## Chunk 1: TDD Contract

### Task 1: Require ELF PHDR marker in x86 smoke output

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add x86 marker requirement for `kernel_phdr_nonzero=1`**
- [x] **Step 2: Run `pytest -q tests/test_boot_handoff_smoke.py` and confirm red before implementation**

## Chunk 2: Boot Implementation

### Task 2: Validate PHDR metadata and seed ELF header fields

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`

- [x] **Step 1: Add nonzero checks for `e_phoff` and `e_phnum` in Stage2 ELF header validation**
- [x] **Step 2: Add `kernel_phdr_nonzero=1` to x86 success marker**
- [x] **Step 3: Seed header sector bytes for `e_phoff` and `e_phnum` in image builder**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase13-x86-elf-phdr-check.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_handoff_smoke.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
