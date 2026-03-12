# Phase 72 Socket Port Table Lifecycle Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align socket-layer port allocation lifecycle with the design requirement for a port table over `1024..65535`, including reuse-after-close behavior.

**Architecture:** Add RED tests for UDP/TCP port reuse after socket close; implement socket port bitmap reservation/release in syscall layers; add net-layer unbind APIs for UDP and TCP listeners; keep C/Rust parity.

**Tech Stack:** C kernel syscall/net modules, Rust kernel crate, pytest, cargo test, cargo check, make

---

## Chunk 1: TDD

### Task 1: Add failing socket lifecycle tests

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C test asserting UDP and TCP listener port can be rebound after close**
- [x] **Step 2: Add Rust test asserting UDP and TCP listener port can be rebound after close**
- [x] **Step 3: Run targeted tests and confirm RED**

## Chunk 2: Implementation (C)

### Task 2: Implement socket port-table lifecycle in C

**Files:**
- Modify: `c-os/kernel/net/net.h`
- Modify: `c-os/kernel/net/net.c`
- Modify: `c-os/kernel/syscall/syscall.c`

- [x] **Step 1: Add UDP unbind + TCP unlisten helpers in net module**
- [x] **Step 2: Add socket-layer port bitmap reservation/release helpers in syscall module**
- [x] **Step 3: Reserve ports in bind/connect; release ports in close when no sibling FD still uses port**

## Chunk 3: Implementation (Rust)

### Task 3: Implement socket port-table lifecycle in Rust

**Files:**
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add `net_udp_unbind` + `net_tcp_unlisten` helpers**
- [x] **Step 2: Add socket-layer port bitmap reservation/release helpers in syscall state**
- [x] **Step 3: Reserve ports in bind/connect; release ports in close when no sibling FD still uses port**

## Chunk 4: Verification

### Task 4: Full verification

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase72-socket-port-table-lifecycle.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cd rust-os && cargo test -q -p qos-kernel --test syscall_core_remaining`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cd rust-os && cargo test -q -p qos-kernel`**
- [x] **Step 5: Run `cd rust-os && cargo test -q -p qos-libc`**
- [x] **Step 6: Run `cd rust-os && cargo check --workspace`**
- [x] **Step 7: Run `make test-all`**
