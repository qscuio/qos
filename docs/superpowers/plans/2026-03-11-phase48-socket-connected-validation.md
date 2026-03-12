# Phase 48 Socket Connected-State Validation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ensure socket data-path syscalls (`send/recv`) require a connected socket state.

**Architecture:** Keep existing socket creation/bind/listen/connect/accept mechanics; add explicit connected-state checks in data-path dispatch so unconnected sockets reject I/O with `-1`.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing unconnected-socket assertions

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C assertions that `send/recv` on unconnected socket return `-1`**
- [x] **Step 2: Add Rust assertions that `send/recv` on unconnected socket return `-1`**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel core_remaining` and confirm red**

## Chunk 2: Implementation

### Task 2: Enforce connected-state checks

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C connected checks in socket data-path operations**
- [x] **Step 2: Add Rust connected checks in socket data-path operations**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase48-socket-connected-validation.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel core_remaining`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
