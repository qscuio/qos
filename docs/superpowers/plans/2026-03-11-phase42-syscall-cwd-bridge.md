# Phase 42 Syscall CWD Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge `chdir(27)` and `getcwd(28)` syscalls in both C and Rust kernels.

**Architecture:** Add per-process cwd state with default `/` and fork inheritance, expose proc cwd set/get primitives, and map syscall dispatch for `chdir`/`getcwd` to VFS existence checks + proc cwd operations.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing cwd syscall tests

**Files:**
- Create: `tests/test_kernel_syscall_cwd.py`
- Create: `rust-os/kernel/tests/syscall_cwd.rs`

- [x] **Step 1: Add C tests for `chdir/getcwd` success + invalid cases**
- [x] **Step 2: Add Rust tests for `chdir/getcwd` success + invalid cases**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_cwd.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel cwd` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement proc cwd state and syscall bridge

**Files:**
- Modify: `c-os/kernel/proc/proc.h`
- Modify: `c-os/kernel/proc/proc.c`
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`
- Modify: `tests/test_kernel_syscall_cwd.py`

- [x] **Step 1: Add C proc cwd constants/APIs and state storage**
- [x] **Step 2: Initialize cwd on create/remove and inherit cwd on fork in C proc state**
- [x] **Step 3: Add syscall op/number constants and default registration for `chdir/getcwd` in C**
- [x] **Step 4: Route C dispatch for `chdir/getcwd` using VFS + proc cwd APIs**
- [x] **Step 5: Add Rust proc cwd state/storage + helpers**
- [x] **Step 6: Add Rust syscall constants/default registration and dispatch for `chdir/getcwd`**
- [x] **Step 7: Fix C test path-buffer lifetime to avoid temporary-pointer invalidation**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase42-syscall-cwd-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_cwd.py tests/test_kernel_syscall_vfs_path.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
