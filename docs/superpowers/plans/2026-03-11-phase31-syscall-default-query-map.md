# Phase 31 Syscall Default Query Map Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a default syscall handler map that exposes live kernel/subsystem counters through stable syscall numbers in both C and Rust kernels.

**Architecture:** Extend syscall dispatch with a query operation kind and selector IDs, reserve syscall numbers for built-in queries (init-state, PMM, scheduler, VFS, net queue, drivers, syscall count, proc table), and register these defaults during `syscall_init`.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing tests for default query-map behavior

**Files:**
- Create: `tests/test_kernel_syscall_defaults.py`
- Create: `rust-os/kernel/tests/syscall_defaults.rs`

- [x] **Step 1: Add C test validating query syscalls after `qos_kernel_entry` and after state mutation**
- [x] **Step 2: Add Rust test validating query syscalls after `kernel_entry` and after state mutation**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_defaults.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel syscall_default_query_map_tracks_live_state` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement syscall query op + default map in C and Rust

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`
- Modify: `tests/test_kernel_status.py`
- Modify: `rust-os/kernel/tests/status.rs`

- [x] **Step 1: Add `SYSCALL_OP_QUERY` plus query selector/number constants in C and Rust**
- [x] **Step 2: Implement query dispatch path returning live subsystem values**
- [x] **Step 3: Register default query handlers during `syscall_init` in C and Rust**
- [x] **Step 4: Update status tests to account for default syscall baseline count**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase31-syscall-default-query-map.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_defaults.py tests/test_kernel_status.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
