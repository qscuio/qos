# Phase 74 Stream Accept Pending Queue Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align passive-open behavior with multi-connection `listen/accept` semantics by tracking multiple pending peers per listener instead of a single overwritten slot.

**Architecture:** Add RED tests with two clients connecting before accept; implement bounded per-listener pending queue (peer port/ip/conn) in syscall state for both C and Rust; keep existing accept sockaddr output behavior.

**Tech Stack:** C syscall module, Rust kernel syscall path, pytest, cargo test

---

## Chunk 1: TDD

### Task 1: Add RED tests for multi-pending accept

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C test with two stream clients connecting before accept; assert two accepts return distinct peer ports**
- [x] **Step 2: Add Rust test with same scenario and assertions**
- [x] **Step 3: Run targeted tests and confirm RED**

## Chunk 2: Implementation

### Task 2: Add per-listener pending peer queue

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add queue state fields (head/tail + per-slot peer metadata)**
- [x] **Step 2: Enqueue peer metadata on stream connect; reject when queue full**
- [x] **Step 3: Dequeue metadata on accept and preserve FIFO order**
- [x] **Step 4: Ensure fd alloc/reset/dup/clear paths keep queue state consistent**

## Chunk 3: Verification

### Task 3: Verify targeted and full regressions

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase74-stream-accept-pending-queue.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cd rust-os && cargo test -q -p qos-kernel --test syscall_core_remaining`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cd rust-os && cargo test -q -p qos-kernel`**
