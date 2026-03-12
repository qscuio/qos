# Phase 39 Syscall Sigreturn Bridge Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge syscall `32 (sigreturn)` in both C and Rust kernels using the current deterministic signal-state model.

**Architecture:** Register syscall number `32` with a `sigreturn` op that restores a process signal mask from syscall arguments (`pid`, `saved_mask`) via proc signal mask APIs.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing sigreturn tests

**Files:**
- Create: `tests/test_kernel_syscall_sigreturn.py`
- Create: `rust-os/kernel/tests/syscall_sigreturn.rs`

- [x] **Step 1: Add C tests for sigreturn mask restore and invalid pid**
- [x] **Step 2: Add Rust tests for sigreturn mask restore and invalid pid**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall_sigreturn.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel sigreturn` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement sigreturn syscall op routing

**Files:**
- Modify: `c-os/kernel/syscall/syscall.h`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add syscall op constant and syscall number constant for `sigreturn (32)`**
- [x] **Step 2: Register default `sigreturn` handler during syscall init**
- [x] **Step 3: Implement C `sigreturn` dispatch (restore mask)**
- [x] **Step 4: Implement Rust `sigreturn` dispatch (restore mask)**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase39-syscall-sigreturn-bridge.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_sigreturn.py tests/test_kernel_syscall_sigsuspend.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
