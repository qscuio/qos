# Phase 33 Sync Primitive Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal kernel-internal synchronization primitives (`spinlock`, `mutex`, `semaphore`) for both C and Rust implementations with deterministic semantics suitable for single-core smoke/runtime tests.

**Architecture:** Add C sync module (`spin`, `mutex`, `sem`) and Rust counterparts (`SpinLock`, `KernelMutex`, `KernelSemaphore`) with non-blocking deterministic wait accounting (`would block` increments waiter count for mutex/semaphore APIs).

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing sync tests

**Files:**
- Create: `tests/test_kernel_sync.py`
- Create: `rust-os/kernel/tests/sync.rs`

- [x] **Step 1: Add C tests for spinlock flow and mutex/semaphore flow**
- [x] **Step 2: Add Rust tests for spinlock flow and mutex/semaphore flow**
- [x] **Step 3: Run `pytest -q tests/test_kernel_sync.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel sync` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement C and Rust sync primitives

**Files:**
- Create: `c-os/kernel/sync/sync.h`
- Create: `c-os/kernel/sync/sync.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C `qos_spin_*` APIs**
- [x] **Step 2: Implement C `qos_mutex_*` APIs**
- [x] **Step 3: Implement C `qos_sem_*` APIs**
- [x] **Step 4: Implement Rust `SpinLock` + `spin_*` APIs**
- [x] **Step 5: Implement Rust `KernelMutex` + `mutex_*` APIs**
- [x] **Step 6: Implement Rust `KernelSemaphore` + `sem_*` APIs**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase33-sync-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_sync.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
