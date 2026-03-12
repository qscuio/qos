# Phase 73 Accept Peer Sockaddr Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align `accept(fd, addr*, addrlen*)` behavior with socket API expectations by returning peer sockaddr metadata for established pending connections.

**Architecture:** Update core syscall tests to assert non-zero `addrlen` and peer endpoint fields; add per-listener pending peer metadata storage in C and Rust syscall state; populate accepted socket remote endpoint and output sockaddr on accept.

**Tech Stack:** C syscall module, Rust kernel syscall path, pytest, cargo test

---

## Chunk 1: TDD

### Task 1: Add RED assertions for accept peer sockaddr

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Change accept assertions from `addrlen==0` to expected sockaddr output (`addrlen==16`, peer port/IP fields)**
- [x] **Step 2: Run targeted tests and confirm RED**

## Chunk 2: Implementation (C + Rust)

### Task 2: Implement pending peer metadata flow into accept

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add pending peer endpoint metadata fields to syscall state**
- [x] **Step 2: Store metadata on stream connect when listener pending queue is incremented**
- [x] **Step 3: Consume metadata on accept and write sockaddr/addrlen outputs**

## Chunk 3: Verification

### Task 3: Verify targeted and full regressions

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase73-accept-peer-sockaddr-alignment.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cd rust-os && cargo test -q -p qos-kernel --test syscall_core_remaining`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cd rust-os && cargo test -q -p qos-kernel`**
