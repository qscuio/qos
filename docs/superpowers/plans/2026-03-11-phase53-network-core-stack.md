# Phase 53 Network Core Stack Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement design-doc network core primitives (ARP cache, IPv4 route selection, UDP demux, TCP state machine) for both C and Rust kernels.

**Architecture:** Extend `net` modules from packet FIFO-only to include protocol state tables. Keep existing queue API unchanged while adding ARP, route, UDP, and TCP APIs. Mirror behavior in C and Rust with equivalent tests.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing network-core tests

**Files:**
- Modify: `tests/test_kernel_net.py`
- Modify: `rust-os/kernel/tests/net.rs`

- [x] **Step 1: Add C tests for ARP TTL/lookup, IPv4 route, UDP demux, TCP transitions**
- [x] **Step 2: Add Rust tests for ARP TTL/lookup, IPv4 route, UDP demux, TCP transitions**
- [x] **Step 3: Run `pytest -q tests/test_kernel_net.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel --test net` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement network-core APIs in C and Rust

**Files:**
- Modify: `c-os/kernel/net/net.h`
- Modify: `c-os/kernel/net/net.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C API/types/constants for ARP, IPv4 route, UDP demux, TCP FSM**
- [x] **Step 2: Implement C ARP cache with TTL aging + lookup/count**
- [x] **Step 3: Implement C IPv4 route selection (local subnet vs gateway)**
- [x] **Step 4: Implement C UDP bind/send/recv demux by destination port**
- [x] **Step 5: Implement C TCP listener/connect/state machine transitions**
- [x] **Step 6: Extend Rust net state with ARP/UDP/TCP tables and constants**
- [x] **Step 7: Implement Rust ARP cache with TTL aging + lookup/count**
- [x] **Step 8: Implement Rust IPv4 route selection (local subnet vs gateway)**
- [x] **Step 9: Implement Rust UDP bind/send/recv demux by destination port**
- [x] **Step 10: Implement Rust TCP listener/connect/state machine transitions**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase53-network-core-stack.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_net.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel --test net`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo test -q -p qos-kernel`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
