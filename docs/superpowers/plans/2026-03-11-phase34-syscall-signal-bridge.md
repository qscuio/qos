# Phase 34 Syscall Signal Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge syscall dispatch to process-signal primitives for `kill` and `sigpending` in both C and Rust kernels.

**Architecture:** Add syscall op kinds and default syscall-number registrations for signal operations (`29=kill`, `33=sigpending`) and route dispatch to proc-signal state APIs.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing syscall-signal tests

**Files:**
- Create: `tests/test_kernel_syscall_signal.py`
- Create: `rust-os/kernel/tests/syscall_signal.rs`

- [x] **Step 1: Add C tests for `kill`/`sigpending` dispatch behavior and invalid targets**
- [x] **Step 2: Add Rust tests for `kill`/`sigpending` dispatch behavior and invalid targets**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_signal.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel syscall_kill` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement syscall signal op kinds and default map

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add syscall op constants for signal bridge and syscall number constants (`29`, `33`)**
- [x] **Step 2: Register default signal syscall handlers during `syscall_init`**
- [x] **Step 3: Route C syscall dispatch to proc-signal APIs for kill/sigpending**
- [x] **Step 4: Route Rust syscall dispatch to proc-signal APIs for kill/sigpending**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase34-syscall-signal-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_signal.py tests/test_kernel_syscall.py tests/test_kernel_syscall_defaults.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
