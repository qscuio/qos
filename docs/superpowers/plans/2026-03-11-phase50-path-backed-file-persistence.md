# Phase 50 Path-Backed File Persistence Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make regular-file data persist across close/reopen for the same path.

**Architecture:** Introduce syscall-local path-backed file store keyed by absolute path. Each VFS fd keeps an offset plus file-slot id; reads/writes/lseek operate on shared file-slot buffer so reopening the same path observes prior writes.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing reopen-persistence assertions

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C test assertions for write->close->reopen->read persistence**
- [x] **Step 2: Add Rust test assertions for write->close->reopen->read persistence**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel core_remaining` and confirm red**

## Chunk 2: Implementation

### Task 2: Add path-backed file store and fd binding

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C file-store tables + fd file-id wiring/reset/clone**
- [x] **Step 2: Update C open/read/write/lseek to use shared path-backed file slots**
- [x] **Step 3: Add Rust file-store tables + fd file-id wiring/reset/clone**
- [x] **Step 4: Update Rust open/read/write/lseek to use shared path-backed file slots**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase50-path-backed-file-persistence.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel core_remaining`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
