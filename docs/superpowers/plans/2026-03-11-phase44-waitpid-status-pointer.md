# Phase 44 Waitpid Status Pointer Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ensure syscall `waitpid(pid, status*, options)` writes exit status into `status*` when provided.

**Architecture:** Keep existing wait matching/removal semantics and WNOHANG behavior; extend syscall bridge to forward status pointer (C) and add a Rust proc helper that returns `(pid, status)` to support user-memory status writeback.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing waitpid status-writeback tests

**Files:**
- Modify: `tests/test_kernel_syscall_waitpid.py`
- Modify: `rust-os/kernel/tests/syscall_waitpid.rs`

- [x] **Step 1: Add C test assertions for `status*` writeback on waited child**
- [x] **Step 2: Add Rust test assertions for `status*` writeback on waited child**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_waitpid.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel waitpid` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Wire status pointer in syscall dispatch

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Forward C syscall `a2` as `int32_t*` to `qos_proc_wait`**
- [x] **Step 2: Add Rust proc wait helper returning pid+status**
- [x] **Step 3: Update Rust syscall waitpid dispatch to write `status*` when non-null**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase44-waitpid-status-pointer.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_waitpid.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel waitpid` in `rust-os/`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 6: Run `make test-all`**
