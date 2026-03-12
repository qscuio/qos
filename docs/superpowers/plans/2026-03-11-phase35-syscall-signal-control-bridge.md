# Phase 35 Syscall Signal Control Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge syscall dispatch for signal-control syscalls `sigaction (30)` and `sigprocmask (31)` in both C and Rust kernels.

**Architecture:** Add new syscall op kinds and default registrations for numbers `30` and `31`. Dispatch routes to proc-signal APIs: `sigaction` returns old handler and optionally sets a new one; `sigprocmask` applies BLOCK/UNBLOCK/SETMASK and returns old mask.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing signal-control syscall tests

**Files:**
- Create: `tests/test_kernel_syscall_signal_ctl.py`
- Create: `rust-os/kernel/tests/syscall_signal_ctl.rs`

- [x] **Step 1: Add C tests for `sigaction`/`sigprocmask` bridge behavior**
- [x] **Step 2: Add Rust tests for `sigaction`/`sigprocmask` bridge behavior**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_signal_ctl.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel syscall_sigaction_and_sigprocmask_bridge` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement signal-control syscall ops

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add syscall op constants for `sigaction` and `sigprocmask`**
- [x] **Step 2: Add syscall number constants `30` and `31` and mask-mode constants**
- [x] **Step 3: Register default `sigaction`/`sigprocmask` handlers during `syscall_init`**
- [x] **Step 4: Route C syscall dispatch to proc-signal handler/mask APIs**
- [x] **Step 5: Route Rust syscall dispatch to proc-signal handler/mask APIs**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase35-syscall-signal-control-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_signal_ctl.py tests/test_kernel_syscall_signal.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
