# Phase 27 Driver Registry Primitive Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal driver-registry primitives for register/query/count in both C and Rust kernels.

**Architecture:** Add fixed-capacity in-memory driver-name tables with duplicate prevention and basic non-empty name validation. Keep `drivers_init` as subsystem marker while resetting registry state for deterministic init behavior.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing driver-registry tests

**Files:**
- Create: `tests/test_kernel_drivers.py`
- Create: `rust-os/kernel/tests/drivers.rs`

- [x] **Step 1: Add C tests for register/query/count and invalid-name handling**
- [x] **Step 2: Add Rust tests for register/query/count and invalid-name handling**
- [x] **Step 3: Run `pytest -q tests/test_kernel_drivers.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement driver-registry primitives in C and Rust

**Files:**
- Create: `c-os/kernel/drivers/drivers.h`
- Modify: `c-os/kernel/drivers/drivers.c`
- Modify: `c-os/kernel/kernel.h`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C driver registry APIs (`reset/register/loaded/count`)**
- [x] **Step 2: Wire C `drivers_init` to deterministic reset + init marker**
- [x] **Step 3: Implement Rust driver registry APIs with synchronization**
- [x] **Step 4: Wire Rust `drivers_init` to deterministic reset + init marker**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase27-driver-registry-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_drivers.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
