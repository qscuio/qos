# Phase 47 Getdents Count Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make syscall `getdents(fd, buf, len)` return a deterministic directory-entry count and write it to `buf`.

**Architecture:** Reuse VFS node count as deterministic synthetic entry count in this stage; require valid VFS fd and output buffer, write count as first `u32` when `len >= 4`, and return that count.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Update core tests for getdents count

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Update C test to expect nonzero getdents count and out-buffer writeback**
- [x] **Step 2: Update Rust test to expect nonzero getdents count and out-buffer writeback**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel core_remaining` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement getdents count mapping

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C getdents path using `qos_vfs_count`, return count, and write first `u32`**
- [x] **Step 2: Implement Rust getdents path using `vfs_count`, return count, and write first `u32`**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase47-getdents-count-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel core_remaining`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
