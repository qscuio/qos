# Phase 51 Unlink/Recreate File Data Reset Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ensure `unlink` followed by recreate does not leak stale file contents.

**Architecture:** Decouple path-backed file slot identity from VFS path lifecycle. On unlink, detach file slot from its path and garbage-collect orphaned slots when no open VFS fd references remain.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing unlink/recreate assertions

**Files:**
- Modify: `tests/test_kernel_syscall_vfs_path.py`
- Modify: `rust-os/kernel/tests/syscall_vfs_path.rs`

- [x] **Step 1: Add C test assertions for `mkdir -> open/write/close -> unlink -> mkdir -> open/read == 0`**
- [x] **Step 2: Add Rust test assertions for `mkdir -> open/write/close -> unlink -> mkdir -> open/read == 0`**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_vfs_path.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel unlink_recreate_clears_file_contents` and confirm red**

## Chunk 2: Implementation

### Task 2: Path-detach + orphan GC

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C file-slot reference detection and orphan drop helpers**
- [x] **Step 2: Update C `unlink` path to detach file slot and GC when unreferenced**
- [x] **Step 3: Update C fd close path to GC orphaned file slots when last ref closes**
- [x] **Step 4: Add Rust file-slot reference detection and orphan drop helpers**
- [x] **Step 5: Update Rust `unlink` path to detach file slot and GC when unreferenced**
- [x] **Step 6: Update Rust fd clear path to GC orphaned file slots when last ref closes**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase51-unlink-recreate-clears-file-data.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_vfs_path.py tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel --test syscall_vfs_path --test syscall_core_remaining`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
