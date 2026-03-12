# Phase 46 Stat/Accept Out-Args Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire syscall out-argument writeback for `stat(path, stat*)` and `accept(fd, addr*, addrlen*)`.

**Architecture:** Keep existing success/failure conditions; on success write deterministic stat data (`size=path len`, marker field) when `stat*` is non-null; on accept success write `*addrlen=0` when pointer is non-null.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing out-arg tests

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C assertions for stat output writeback and accept addrlen writeback**
- [x] **Step 2: Add Rust assertions for stat output writeback and accept addrlen writeback**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel core_remaining` and confirm red**

## Chunk 2: Implementation

### Task 2: Wire out-args in dispatch

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C stat writeback (`a1`) and accept addrlen writeback (`a2`)**
- [x] **Step 2: Implement Rust stat writeback (`a1`) and accept addrlen writeback (`a2`)**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase46-stat-accept-outargs.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel core_remaining`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
