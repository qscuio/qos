# Phase 19 Libc/Userspace Stub Removal Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate remaining concrete function stubs/placeholders in libc and userspace entry points with minimal functional implementations and executable tests.

**Architecture:** Replace single-return stubs with small, deterministic helper implementations for C/Rust libc and implement matching shell command loops for C/Rust userspace binaries. Validate behavior with new pytest coverage (including C shared-lib symbol checks and Rust package execution).

**Tech Stack:** C11, Rust (`no_std` for libc crate, `std` for userspace binary), pytest, gcc, cargo

---

## Chunk 1: TDD Contract

### Task 1: Add failing behavior tests for stub replacement

**Files:**
- Create: `tests/test_pending_functions.py`
- Create: `rust-os/libc/tests/basic.rs`

- [x] **Step 1: Add C libc behavior test (strlen/strcmp/memset/memcpy) via shared library**
- [x] **Step 2: Add C shell behavior test (help + exit)**
- [x] **Step 3: Add Rust libc tests requiring helper exports**
- [x] **Step 4: Add Rust userspace one-shot help command test**
- [x] **Step 5: Run `pytest -q tests/test_pending_functions.py` and confirm red**

## Chunk 2: Implementation

### Task 2: Replace libc and userspace stubs

**Files:**
- Modify: `c-os/libc/libc.c`
- Modify: `c-os/userspace/shell.c`
- Modify: `rust-os/libc/src/lib.rs`
- Modify: `rust-os/userspace/src/main.rs`

- [x] **Step 1: Implement C libc helper functions (`qos_strlen`, `qos_strcmp`, `qos_memset`, `qos_memcpy`)**
- [x] **Step 2: Implement C userspace shell command loop with `help`, `echo`, `exit`**
- [x] **Step 3: Implement Rust libc helper functions with `no_std` compatibility**
- [x] **Step 4: Implement Rust userspace shell command handling with `--once` mode**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase19-libc-userspace-stub-removal.md`

- [x] **Step 1: Run `pytest -q tests/test_pending_functions.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
