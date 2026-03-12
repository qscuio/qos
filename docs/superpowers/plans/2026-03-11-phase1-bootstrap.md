# Phase 1 Bootstrap Scaffold Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a runnable monorepo scaffold for `c-os/` and `rust-os/` with top-level build delegation and a deterministic `make test-all` smoke matrix.

**Architecture:** Keep this phase intentionally minimal: generate placeholder kernel artifacts per implementation/architecture, route all orchestration through top-level Make targets, and run smoke checks via shell scripts that validate boot-marker logs. This creates stable CI plumbing now and leaves room to replace placeholders with real boot/QEMU execution later.

**Tech Stack:** GNU Make, Bash, Python/pytest

---

## Chunk 1: Scaffold Contracts + TDD

### Task 1: Add failing tests for scaffold behavior

**Files:**
- Create: `tests/test_phase1_scaffold.py`

- [x] **Step 1: Write failing tests**
- [x] **Step 2: Run `pytest -q tests/test_phase1_scaffold.py` and confirm failure**
- [x] **Step 3: Implement minimal build/smoke scaffold**
- [x] **Step 4: Re-run tests and confirm pass**

### Task 2: Implement monorepo orchestration

**Files:**
- Create: `Makefile`
- Create: `c-os/Makefile`
- Create: `rust-os/Makefile`
- Create: `scripts/smoke_target.sh`
- Create: `qemu/x86_64.sh`
- Create: `qemu/aarch64.sh`
- Create: `qemu/configs/.gitkeep`

- [x] **Step 1: Add top-level delegated targets (`make c x86_64`, `make rust aarch64`, `make test-all`)**
- [x] **Step 2: Add implementation-specific placeholder build targets**
- [x] **Step 3: Add smoke script that emits and validates boot marker**
- [x] **Step 4: Add QEMU command wrapper scripts (dry-run command builders)**

## Chunk 2: Verification

### Task 3: End-to-end verification

**Files:**
- Modify: `tests/test_phase1_scaffold.py` (only if needed)

- [x] **Step 1: Run targeted scaffold tests**
- [x] **Step 2: Run full `pytest -q`**
- [x] **Step 3: Run `make test-all` and verify all 4 matrix entries pass**
