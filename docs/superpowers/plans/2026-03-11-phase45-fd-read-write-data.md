# Phase 45 FD Read/Write Data Semantics Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `open/read/write/lseek` on regular fd entries return and consume in-memory bytes instead of placeholder zero-byte reads.

**Architecture:** Extend syscall-local fd state with per-fd file buffer and length; implement write-at-offset growth and read-at-offset consumption semantics; make `lseek(..., SEEK_END)` use current file length.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Turn existing core syscall test into data-flow expectation

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Update C test to expect `read` returns written bytes after `lseek(0)`**
- [x] **Step 2: Update Rust test to expect `read` returns written bytes after `lseek(0)`**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel core_remaining` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Add in-memory fd file buffers

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C fd file-buffer storage and reset/clone wiring**
- [x] **Step 2: Implement C `read/write/lseek` regular-file paths using buffer+offset+length**
- [x] **Step 3: Add Rust fd file-buffer storage and reset/clone wiring**
- [x] **Step 4: Implement Rust `read/write/lseek` regular-file paths using buffer+offset+length**

## Chunk 3: Verification

### Task 3: Full checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase45-fd-read-write-data.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel core_remaining` in `rust-os/`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 6: Run `make test-all`**
