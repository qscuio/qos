# Phase 41 Syscall VFS Path Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge filesystem path syscalls `stat(21)`, `mkdir(22)`, and `unlink(23)` to existing VFS primitives in both C and Rust kernels.

**Architecture:** Add default syscall handlers for numbers `21/22/23` and route dispatch to VFS path operations (`exists/create/remove`). For C, keep standalone syscall-library tests stable by using weak links for VFS symbols.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing vfs-path syscall tests

**Files:**
- Create: `tests/test_kernel_syscall_vfs_path.py`
- Create: `rust-os/kernel/tests/syscall_vfs_path.rs`

- [x] **Step 1: Add C tests for `stat/mkdir/unlink` success/failure flows**
- [x] **Step 2: Add Rust tests for `stat/mkdir/unlink` success/failure flows**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_vfs_path.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel vfs_path` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement vfs-path syscall ops and default map

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add syscall op constants and syscall number constants for `21/22/23`**
- [x] **Step 2: Register default handlers for `stat/mkdir/unlink` during syscall init**
- [x] **Step 3: Implement C dispatch routing for path syscalls with weak VFS symbols**
- [x] **Step 4: Implement Rust dispatch routing for path syscalls with C-string decoding**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase41-syscall-vfs-path-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_vfs_path.py tests/test_kernel_syscall_waitpid.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
