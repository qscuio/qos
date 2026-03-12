# Phase 69 TCP RTO Cap Observability Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make TCP RTO cap behavior (max 60s) directly testable and guarded in both C and Rust network stacks.

**Architecture:** Add lightweight helper APIs exposing the RTO progression function and add mirrored tests validating backoff progression and cap behavior.

**Tech Stack:** C net module, Rust kernel net module, pytest, cargo test

---

## Chunk 1: TDD

### Task 1: Add RTO cap tests

**Files:**
- Modify: `tests/test_kernel_net.py`
- Modify: `rust-os/kernel/tests/net.rs`

- [x] **Step 1: Add C test for helper progression and 60s cap**
- [x] **Step 2: Add Rust mirror test for helper progression and 60s cap**
- [x] **Step 3: Run targeted tests and confirm RED (missing symbols/APIs)**

## Chunk 2: Implementation

### Task 2: Expose RTO helper APIs

**Files:**
- Modify: `c-os/kernel/net/net.h`
- Modify: `c-os/kernel/net/net.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C API `qos_net_tcp_next_rto_ms`**
- [x] **Step 2: Add Rust API `net_tcp_next_rto_ms`**
- [x] **Step 3: Re-run targeted tests and confirm GREEN**

## Chunk 3: Regression Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase69-tcp-rto-cap-observability.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
