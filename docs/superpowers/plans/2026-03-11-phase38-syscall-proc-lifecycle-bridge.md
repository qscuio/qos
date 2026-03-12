# Phase 38 Syscall Proc Lifecycle Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge syscall `fork(0)` and `exec(1)` to process-table lifecycle primitives in both C and Rust kernels.

**Architecture:** Register default syscall handlers for `0` and `1`; route `fork` to proc cloning (`parent_pid`, `child_pid`) and `exec` to signal-state exec reset for a target pid. Preserve deterministic behavior compatible with current proc signal semantics tests.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing lifecycle syscall tests

**Files:**
- Create: `tests/test_kernel_syscall_proc_lifecycle.py`
- Create: `rust-os/kernel/tests/syscall_proc_lifecycle.rs`

- [x] **Step 1: Add C tests for `fork`/`exec` syscall dispatch and proc-signal semantics**
- [x] **Step 2: Add Rust tests for `fork`/`exec` syscall dispatch and proc-signal semantics**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_proc_lifecycle.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel syscall_fork_exec` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement lifecycle syscall ops and default registration

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add syscall op constants and number constants for `fork(0)` and `exec(1)`**
- [x] **Step 2: Register default `fork`/`exec` handlers during syscall init**
- [x] **Step 3: Implement C `fork`/`exec` dispatch routing to proc lifecycle APIs**
- [x] **Step 4: Implement Rust `fork`/`exec` dispatch routing to proc lifecycle APIs**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase38-syscall-proc-lifecycle-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_proc_lifecycle.py tests/test_kernel_syscall.py tests/test_kernel_syscall_defaults.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
