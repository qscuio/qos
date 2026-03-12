# Phase 24 Scheduler Primitive Run Queue Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal scheduler primitives (task add/remove/next) with deterministic round-robin behavior in both C and Rust kernels.

**Architecture:** Add fixed-capacity runnable PID queues with duplicate prevention and round-robin pointer management. Keep `sched_init` as subsystem init marker while wiring scheduler reset for deterministic kernel-entry behavior.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing scheduler behavior tests

**Files:**
- Create: `tests/test_kernel_sched.py`
- Create: `rust-os/kernel/tests/sched.rs`

- [x] **Step 1: Add C tests for round-robin order and duplicate/remove behavior**
- [x] **Step 2: Add Rust tests for round-robin order and duplicate/remove behavior**
- [x] **Step 3: Run `pytest -q tests/test_kernel_sched.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement scheduler run queue in C and Rust

**Files:**
- Create: `c-os/kernel/sched/sched.h`
- Modify: `c-os/kernel/sched/sched.c`
- Modify: `c-os/kernel/kernel.h`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C scheduler queue APIs (`reset/add/remove/count/next`)**
- [x] **Step 2: Wire C `sched_init` to preserve init marker and deterministic reset**
- [x] **Step 3: Implement Rust scheduler queue APIs with synchronization for tests**
- [x] **Step 4: Wire Rust `sched_init` to reset queue and mark init bit**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase24-scheduler-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_sched.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
