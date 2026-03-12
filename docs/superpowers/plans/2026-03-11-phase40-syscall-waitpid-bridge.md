# Phase 40 Syscall Exit/Getpid/Waitpid Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge lifecycle syscalls `exit(2)`, `getpid(3)`, and `waitpid(4)` in both C and Rust kernels.

**Architecture:** Extend proc state with exited/exit-code tracking and waiting primitive; register default syscall handlers for `2/3/4`; route dispatch to proc lifecycle APIs (`exit`, `alive/getpid`, `wait`).

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing lifecycle syscall tests

**Files:**
- Create: `tests/test_kernel_syscall_waitpid.py`
- Create: `rust-os/kernel/tests/syscall_waitpid.rs`

- [x] **Step 1: Add C tests for `exit/getpid/waitpid` behavior and invalid parent**
- [x] **Step 2: Add Rust tests for `exit/getpid/waitpid` behavior and invalid parent**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_waitpid.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel waitpid` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement proc exit/wait primitives and syscall bridge

**Files:**
- Modify: `c-os/kernel/proc/proc.h`
- Modify: `c-os/kernel/proc/proc.c`
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add proc exit/wait declarations and `QOS_WAIT_WNOHANG` constant in C**
- [x] **Step 2: Add C proc exited/exit-code state and implement `qos_proc_exit` / `qos_proc_wait`**
- [x] **Step 3: Add syscall op/number constants and default registration for `exit/getpid/waitpid` in C**
- [x] **Step 4: Route C syscall dispatch for `exit/getpid/waitpid`**
- [x] **Step 5: Add Rust proc exited/exit-code state and implement `proc_exit` / `proc_wait`**
- [x] **Step 6: Add Rust syscall op/number constants and default registration for `exit/getpid/waitpid`**
- [x] **Step 7: Route Rust syscall dispatch for `exit/getpid/waitpid`**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase40-syscall-waitpid-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_waitpid.py tests/test_kernel_proc.py tests/test_kernel_syscall_proc_lifecycle.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
