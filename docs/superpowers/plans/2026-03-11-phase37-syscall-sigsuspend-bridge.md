# Phase 37 Syscall Sigsuspend Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge syscall `34 (sigsuspend)` to process signal state in both C and Rust kernels.

**Architecture:** Add a default syscall op for number `34` that applies a temporary signal mask, attempts delivery of one pending unmasked signal, and restores the prior mask before returning the delivered signal number (or `0` if none).

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing sigsuspend tests

**Files:**
- Create: `tests/test_kernel_syscall_sigsuspend.py`
- Create: `rust-os/kernel/tests/syscall_sigsuspend.rs`

- [x] **Step 1: Add C tests for `sigsuspend` temporary-mask behavior and invalid pid**
- [x] **Step 2: Add Rust tests for `sigsuspend` temporary-mask behavior and invalid pid**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_sigsuspend.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel sigsuspend` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement sigsuspend syscall op routing

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add syscall op and syscall number constants for `sigsuspend` (`34`)**
- [x] **Step 2: Register default `sigsuspend` handler during syscall init**
- [x] **Step 3: Implement C `sigsuspend` dispatch (temp mask, next signal, restore mask)**
- [x] **Step 4: Implement Rust `sigsuspend` dispatch (temp mask, next signal, restore mask)**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase37-syscall-sigsuspend-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_sigsuspend.py tests/test_kernel_syscall_signal_ctl.py tests/test_kernel_syscall_sigaltstack.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
