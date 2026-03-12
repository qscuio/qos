# Phase 49 Socket Handshake Validation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enforce minimal handshake semantics for `connect/accept`.

**Architecture:** Track pending connection count per listening socket in syscall-local socket state. `connect` succeeds only when some listener exists and queues one pending connection; `accept` succeeds only if listener has pending connection(s), then consumes one.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing handshake assertions

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C assertions: `connect` fails without listener, `accept` fails before connect**
- [x] **Step 2: Add Rust assertions: `connect` fails without listener, `accept` fails before connect**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel core_remaining` and confirm red**

## Chunk 2: Implementation

### Task 2: Add pending-connection tracking

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C socket pending-connection state/reset/clone wiring**
- [x] **Step 2: Update C `connect` to require listener and queue pending connection**
- [x] **Step 3: Update C `accept` to require and consume pending connection**
- [x] **Step 4: Add Rust socket pending-connection state/reset/clone wiring**
- [x] **Step 5: Update Rust `connect` to require listener and queue pending connection**
- [x] **Step 6: Update Rust `accept` to require and consume pending connection**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase49-socket-handshake-validation.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel core_remaining`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
