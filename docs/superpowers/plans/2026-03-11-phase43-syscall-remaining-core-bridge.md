# Phase 43 Remaining Core Syscalls Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the remaining architecture-specified syscall numbers (`5-20`, `24-26`) in both C and Rust kernels.

**Architecture:** Extend syscall op/number maps and dispatch to existing kernel primitives (`vfs`, `vmm`, `sched`, `net`) and a minimal in-kernel fd/socket/pipe table inside syscall state. Preserve current deterministic behavior style: explicit argument validation and `-1` on invalid input.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing syscall bridge tests for missing numbers

**Files:**
- Create: `tests/test_kernel_syscall_core_remaining.py`
- Create: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C tests covering fd/file io syscalls (`open/read/write/close/dup2/getdents/lseek/pipe`)**
- [x] **Step 2: Add C tests covering mm/sched/net syscalls (`mmap/munmap/yield/sleep/socket/bind/listen/accept/connect/send/recv`)**
- [x] **Step 3: Add Rust tests mirroring fd/file io and mm/sched/net syscall expectations**
- [x] **Step 4: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 5: Run `cargo test -q -p qos-kernel core_remaining` in `rust-os/` and confirm red**

## Chunk 2: C Kernel Implementation

### Task 2: Add C syscall ops/numbers, state, and dispatch

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`

- [x] **Step 1: Add missing C syscall op constants and syscall number constants (`5-20`, `24-26`)**
- [x] **Step 2: Register new defaults in C syscall initialization**
- [x] **Step 3: Extend C syscall register validation to accept new op kinds**
- [x] **Step 4: Add C syscall-local fd/socket/pipe state reset helpers**
- [x] **Step 5: Implement C dispatch for fd/file io operations (`open/read/write/close/dup2/getdents/lseek/pipe`)**
- [x] **Step 6: Implement C dispatch for mm/sched ops (`mmap/munmap/yield/sleep`)**
- [x] **Step 7: Implement C dispatch for net socket ops (`socket/bind/listen/accept/connect/send/recv`)**

## Chunk 3: Rust Kernel Implementation

### Task 3: Add Rust syscall ops/numbers, state, and dispatch

**Files:**
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add missing Rust syscall op constants and syscall number constants (`5-20`, `24-26`)**
- [x] **Step 2: Register new defaults in Rust syscall initialization**
- [x] **Step 3: Extend Rust syscall register validation to accept new op kinds**
- [x] **Step 4: Extend `SyscallState` with fd/socket/pipe state and reset behavior**
- [x] **Step 5: Implement Rust dispatch for fd/file io operations (`open/read/write/close/dup2/getdents/lseek/pipe`)**
- [x] **Step 6: Implement Rust dispatch for mm/sched ops (`mmap/munmap/yield/sleep`)**
- [x] **Step 7: Implement Rust dispatch for net socket ops (`socket/bind/listen/accept/connect/send/recv`)**

## Chunk 4: Verification

### Task 4: Full verification and plan update

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase43-syscall-remaining-core-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py tests/test_kernel_syscall_cwd.py tests/test_kernel_syscall_vfs_path.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel core_remaining` in `rust-os/`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 6: Run `make test-all`**
