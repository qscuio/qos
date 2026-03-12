# Phase 28 Syscall Dispatch Primitive Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal syscall dispatch primitives for register/unregister/dispatch in both C and Rust kernels.

**Architecture:** Add fixed-capacity syscall tables keyed by syscall number with simple operation kinds (`CONST`, `ADD`) to validate dispatch routing and argument flow. Keep `syscall_init` as subsystem marker while resetting syscall table for deterministic init behavior.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing syscall behavior tests

**Files:**
- Create: `tests/test_kernel_syscall.py`
- Create: `rust-os/kernel/tests/syscall.rs`

- [x] **Step 1: Add C tests for register/dispatch/unregister and invalid inputs**
- [x] **Step 2: Add Rust tests for register/dispatch/unregister and invalid inputs**
- [x] **Step 3: Run `pytest -q tests/test_kernel_syscall.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement syscall dispatch primitives in C and Rust

**Files:**
- Create: `c-os/kernel/syscall/syscall.h`
- Create: `c-os/kernel/syscall/syscall.c`
- Modify: `c-os/kernel/kernel.h`
- Modify: `c-os/kernel/kernel.c`
- Modify: `tests/test_kernel_init.py`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C syscall table APIs (`reset/register/unregister/dispatch/count`)**
- [x] **Step 2: Wire C `syscall_init` into kernel init sequencing**
- [x] **Step 3: Implement Rust syscall table APIs with synchronization**
- [x] **Step 4: Wire Rust `syscall_init` into kernel init sequencing**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase28-syscall-dispatch-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
