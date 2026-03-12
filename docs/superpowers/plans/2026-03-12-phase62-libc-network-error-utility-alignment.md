# Phase 62 libc Network/Error Utility Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete and verify libc helper coverage for network byte-order/IP parsing and error/process utility functions in both C and Rust implementations.

**Architecture:** Extend existing libc helper surface (`qos_htons/htonl/ntohs/ntohl`, `qos_inet_addr/qos_inet_ntoa`, `qos_errno_*`, `qos_strerror`, `qos_getpid`) and harden copy semantics for `qos_strncpy` edge cases to match bounded-copy behavior.

**Tech Stack:** pytest, cargo test, cargo check, make test-all

---

## Chunk 1: TDD Expansion

### Task 1: Add failing tests for new helper coverage

**Files:**
- Modify: `tests/test_pending_functions.py`
- Modify: `rust-os/libc/tests/basic.rs`

- [x] **Step 1: Add C tests for byte order, inet helpers, errno/strerror, snprintf, getpid**
- [x] **Step 2: Add Rust tests for byte order, inet helpers, errno/strerror, getpid**
- [x] **Step 3: Add `strncpy(n=0)` regression checks in both C and Rust test suites**
- [x] **Step 4: Run tests and confirm RED on current `strncpy` behavior**

## Chunk 2: libc Fixes

### Task 2: Align helper behavior with bounded-copy semantics

**Files:**
- Modify: `c-os/libc/libc.c`
- Modify: `rust-os/libc/src/lib.rs`

- [x] **Step 1: Fix C `qos_strncpy` to avoid out-of-bounds forced terminator writes**
- [x] **Step 2: Fix Rust `qos_strncpy` to avoid forced write for `n=0` and preserve bounded semantics**
- [x] **Step 3: Re-run targeted tests and confirm GREEN**

## Chunk 3: Full Verification

### Task 3: End-to-end regression validation

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase62-libc-network-error-utility-alignment.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all` (sequentially, avoid QEMU lock contention)**
