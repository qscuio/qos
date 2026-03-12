# Phase 30 Kernel Status Snapshot Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a cross-subsystem kernel status snapshot API in both C and Rust so tests can validate integrated kernel state in one call.

**Architecture:** Define a compact status struct containing init-state and subsystem counts (PMM, scheduler, VFS, net queue, drivers, syscalls, proc table), then expose a snapshot function in C and Rust.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing kernel-status tests

**Files:**
- Create: `tests/test_kernel_status.py`
- Create: `rust-os/kernel/tests/status.rs`

- [x] **Step 1: Add C integration test for status snapshot after mutating subsystem state**
- [x] **Step 2: Add Rust integration test for status snapshot after mutating subsystem state**
- [x] **Step 3: Run `pytest -q tests/test_kernel_status.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement status snapshot APIs in C and Rust

**Files:**
- Modify: `c-os/kernel/kernel.h`
- Modify: `c-os/kernel/kernel.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C `qos_kernel_status_t` and `qos_kernel_status_snapshot`**
- [x] **Step 2: Add Rust `KernelStatus` and `kernel_status()`**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase30-kernel-status-snapshot.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_status.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
