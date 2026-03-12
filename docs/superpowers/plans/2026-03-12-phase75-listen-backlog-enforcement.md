# Phase 75 Listen Backlog Enforcement Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enforce `listen(fd, backlog)` semantics so stream `connect` fails when a listener's pending queue depth reaches its configured backlog.

**Architecture:** Add RED tests for backlog=1 behavior in C and Rust; add per-listener backlog state in syscall FD tables; set/clamp backlog during `listen`; reject stream `connect` when pending depth is at backlog.

**Tech Stack:** C syscall module, Rust kernel syscall path, pytest, cargo test, cargo check, make

---

## Chunk 1: TDD

### Task 1: Add failing backlog-limit tests

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C test `test_c_listen_backlog_limits_pending_connects`**
- [x] **Step 2: Add Rust test `core_remaining_listen_backlog_limits_pending_connects`**
- [x] **Step 3: Run targeted tests and confirm RED**

## Chunk 2: Implementation

### Task 2: Track and enforce per-listener backlog

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add per-FD `sock_backlog` state and initialize/reset/clear/copy paths**
- [x] **Step 2: In `listen`, clamp backlog to queue cap and persist backlog for listener sockets**
- [x] **Step 3: In stream `connect`, reject enqueue when listener pending count reaches backlog**

## Chunk 3: Verification

### Task 3: Verify targeted and full regressions

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase75-listen-backlog-enforcement.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py::test_c_listen_backlog_limits_pending_connects`**
- [x] **Step 2: Run `cd rust-os && cargo test -q -p qos-kernel --test syscall_core_remaining core_remaining_listen_backlog_limits_pending_connects`**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 4: Run `cd rust-os && cargo test -q -p qos-kernel --test syscall_core_remaining`**
- [x] **Step 5: Run `pytest -q`**
- [x] **Step 6: Run `cd rust-os && cargo test -q -p qos-kernel`**
- [x] **Step 7: Run `cd rust-os && cargo test -q -p qos-libc`**
- [x] **Step 8: Run `cd rust-os && cargo check --workspace`**
- [x] **Step 9: Run `make test-all`**
