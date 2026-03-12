# Phase 68 TCP Retransmit Policy Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make TCP retransmit behavior explicitly testable and aligned with spec constraints (initial RTO 500ms, exponential backoff, max 5 retries before close) in both C and Rust implementations.

**Architecture:** Add read-only TCP connection introspection APIs (`rto_ms`, `retries`) in C and Rust network stacks, then add tests for timeout-driven backoff progression and close-on-limit behavior.

**Tech Stack:** C net module, Rust kernel net module, pytest, cargo test

---

## Chunk 1: TDD

### Task 1: Add failing retransmit policy tests

**Files:**
- Modify: `tests/test_kernel_net.py`
- Modify: `rust-os/kernel/tests/net.rs`

- [x] **Step 1: Add C test for SYN_SENT timeout progression (500→1000→2000→4000→8000→16000)**
- [x] **Step 2: Add C test assertion that 6th timeout closes after 5 retries**
- [x] **Step 3: Add Rust mirror test for same progression/close semantics**
- [x] **Step 4: Run targeted net tests**

## Chunk 2: Implementation

### Task 2: Expose TCP introspection state

**Files:**
- Modify: `c-os/kernel/net/net.h`
- Modify: `c-os/kernel/net/net.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C APIs `qos_net_tcp_rto` and `qos_net_tcp_retries`**
- [x] **Step 2: Add Rust APIs `net_tcp_rto` and `net_tcp_retries`**
- [x] **Step 3: Re-run targeted tests and confirm GREEN**

## Chunk 3: Regression Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase68-tcp-retransmit-policy-alignment.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
