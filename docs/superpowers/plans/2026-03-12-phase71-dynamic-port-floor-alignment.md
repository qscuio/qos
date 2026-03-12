# Phase 71 Dynamic Port Floor Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enforce the design requirement that dynamic/bindable socket ports are `>= 1024` in both C and Rust network stacks.

**Architecture:** Drive the change with tests that assert low-port rejection across UDP and TCP APIs, then enforce the shared minimum port policy in Rust to match already-updated C behavior.

**Tech Stack:** C kernel net module, Rust kernel net module, pytest, cargo test, cargo check, make

---

## Chunk 1: TDD

### Task 1: Add low-port rejection tests

**Files:**
- Modify: `tests/test_kernel_net.py`
- Modify: `rust-os/kernel/tests/net.rs`

- [x] **Step 1: Add C test for low-port rejection in UDP bind/send and TCP listen/connect**
- [x] **Step 2: Add Rust test for low-port rejection in UDP bind/send and TCP listen/connect**
- [x] **Step 3: Run targeted tests and confirm RED in Rust (`net_udp_bind(53)` accepted pre-fix)**

## Chunk 2: Implementation

### Task 2: Enforce dynamic port floor in Rust net APIs

**Files:**
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add shared port minimum constant (`QOS_NET_PORT_MIN_DYNAMIC = 1024`)**
- [x] **Step 2: Reject ports `< 1024` in `net_udp_bind` and `net_udp_send` source port validation**
- [x] **Step 3: Reject ports `< 1024` in `net_tcp_listen` and `net_tcp_connect` local port validation**
- [x] **Step 4: Re-run targeted C and Rust net suites and confirm GREEN**

## Chunk 3: Regression Verification

### Task 3: Full repository verification

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase71-dynamic-port-floor-alignment.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cd rust-os && cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cd rust-os && cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cd rust-os && cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
