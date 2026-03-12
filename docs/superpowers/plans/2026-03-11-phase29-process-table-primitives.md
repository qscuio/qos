# Phase 29 Process Table Primitive Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal process-table primitives for create/remove/parent/alive/count in both C and Rust kernels.

**Architecture:** Add fixed-capacity process tables keyed by PID with parent-PID tracking and alive status. Keep `proc_init` as subsystem marker while resetting process table for deterministic init behavior.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing process-table tests

**Files:**
- Create: `tests/test_kernel_proc.py`
- Create: `rust-os/kernel/tests/proc.rs`

- [x] **Step 1: Add C tests for create/remove/parent/alive/count and invalid PID handling**
- [x] **Step 2: Add Rust tests for create/remove/parent/alive/count and invalid PID handling**
- [x] **Step 3: Run `pytest -q tests/test_kernel_proc.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement process-table primitives in C and Rust

**Files:**
- Create: `c-os/kernel/proc/proc.h`
- Create: `c-os/kernel/proc/proc.c`
- Modify: `c-os/kernel/kernel.h`
- Modify: `c-os/kernel/kernel.c`
- Modify: `tests/test_kernel_init.py`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C process table APIs (`reset/create/remove/parent/alive/count`)**
- [x] **Step 2: Wire C `proc_init` into kernel init sequencing**
- [x] **Step 3: Implement Rust process table APIs with synchronization**
- [x] **Step 4: Wire Rust `proc_init` into kernel init sequencing**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase29-process-table-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_proc.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
