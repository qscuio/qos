# Phase 26 Net Queue Primitive Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal network queue primitives for packet enqueue/dequeue in both C and Rust kernels.

**Architecture:** Add fixed-capacity FIFO packet queues with max packet size checks and bounded dequeue copy behavior. Keep `net_init` as subsystem marker while resetting queue state for deterministic init behavior.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing net-queue behavior tests

**Files:**
- Create: `tests/test_kernel_net.py`
- Create: `rust-os/kernel/tests/net.rs`

- [x] **Step 1: Add C tests for FIFO enqueue/dequeue and invalid-size handling**
- [x] **Step 2: Add Rust tests for FIFO enqueue/dequeue and invalid-size handling**
- [x] **Step 3: Run `pytest -q tests/test_kernel_net.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement net queue primitives in C and Rust

**Files:**
- Create: `c-os/kernel/net/net.h`
- Modify: `c-os/kernel/net/net.c`
- Modify: `c-os/kernel/kernel.h`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C net queue APIs (`reset/enqueue/dequeue/queue_len`)**
- [x] **Step 2: Wire C `net_init` to deterministic reset + init marker**
- [x] **Step 3: Implement Rust net queue APIs with synchronization**
- [x] **Step 4: Wire Rust `net_init` to deterministic reset + init marker**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase26-net-queue-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_net.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
