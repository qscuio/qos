# Phase 36 Syscall Sigaltstack Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement process altstack state primitives and bridge syscall `35 (sigaltstack)` in both C and Rust kernels.

**Architecture:** Extend proc signal state with altstack fields (`sp`, `size`, `flags`), inherit altstack across `fork`, expose proc-level set/get helpers, and register syscall number `35` as a default signal-control handler that supports query-only (`sp = U64_MAX`) and update mode.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing altstack tests

**Files:**
- Create: `tests/test_kernel_syscall_sigaltstack.py`
- Create: `rust-os/kernel/tests/syscall_sigaltstack.rs`
- Modify: `tests/test_kernel_proc_signal.py`
- Modify: `rust-os/kernel/tests/proc_signal.rs`

- [x] **Step 1: Add C syscall `sigaltstack` bridge test with proc-level state verification**
- [x] **Step 2: Add Rust syscall `sigaltstack` bridge test with proc-level state verification**
- [x] **Step 3: Extend C proc-signal fork semantics test to require altstack inheritance**
- [x] **Step 4: Extend Rust proc-signal fork semantics test to require altstack inheritance**
- [x] **Step 5: Run `pytest -q tests/test_kernel_syscall_sigaltstack.py tests/test_kernel_proc_signal.py` and confirm red**
- [x] **Step 6: Run `cargo test -q -p qos-kernel sigaltstack` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement proc altstack state + syscall bridge

**Files:**
- Modify: `c-os/kernel/proc/proc.h`
- Modify: `c-os/kernel/proc/proc.c`
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C altstack type and proc APIs (`altstack_set/get`)**
- [x] **Step 2: Add C proc altstack storage and fork inheritance behavior**
- [x] **Step 3: Add C syscall op/number constants for `sigaltstack (35)` and dispatch route**
- [x] **Step 4: Add Rust proc altstack storage/type and `proc_signal_set/get_altstack` APIs**
- [x] **Step 5: Add Rust syscall op/number constants and dispatch route for `sigaltstack`**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase36-syscall-sigaltstack-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_proc_signal.py tests/test_kernel_syscall_sigaltstack.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
