# Phase 52 Lseek Overflow Checks Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enforce deterministic `lseek` overflow behavior and prevent undefined arithmetic.

**Architecture:** Add overflow guards in syscall `lseek` path for both kernels. `SEEK_SET/SEEK_CUR/SEEK_END` now return `-1` when `base + offset` overflows signed 64-bit range.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing overflow assertions

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C assertion for `lseek(INT64_MAX, SEEK_SET)` then `lseek(+1, SEEK_CUR) == -1`**
- [x] **Step 2: Add Rust assertion for `lseek(INT64_MAX, SEEK_SET)` then `lseek(+1, SEEK_CUR) == -1`**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 4: Run `cargo test -q -p qos-kernel --test syscall_core_remaining` and confirm red on overflow case**

## Chunk 2: Implementation

### Task 2: Checked arithmetic in `lseek`

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C overflow checks before `base + offset` in `SYSCALL_OP_LSEEK`**
- [x] **Step 2: Switch Rust `lseek` add to `checked_add` and return `-1` on overflow**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase52-lseek-overflow-checks.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel --test syscall_core_remaining`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
