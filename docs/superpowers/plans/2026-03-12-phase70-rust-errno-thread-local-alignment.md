# Phase 70 Rust errno Thread-Local Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align Rust libc `errno` behavior with spec-level thread-local semantics.

**Architecture:** Add thread-local errno tests (C + Rust), switch host Rust errno storage from global atomic to libc thread-local errno pointer (`__errno_location`), and keep atomic fallback for `target_os=none`.

**Tech Stack:** Rust libc crate, pytest, cargo test, cargo check, make

---

## Chunk 1: TDD

### Task 1: Add failing thread-local errno tests

**Files:**
- Modify: `tests/test_pending_functions.py`
- Modify: `rust-os/libc/tests/basic.rs`

- [x] **Step 1: Add C test asserting per-thread errno isolation**
- [x] **Step 2: Add Rust test asserting per-thread errno isolation**
- [x] **Step 3: Run targeted tests and confirm RED in Rust**

## Chunk 2: Implementation

### Task 2: Switch Rust host errno to thread-local storage

**Files:**
- Modify: `rust-os/libc/src/lib.rs`

- [x] **Step 1: Bind libc `__errno_location` for host targets**
- [x] **Step 2: Route `qos_errno_set/get` through thread-local host errno**
- [x] **Step 3: Retain atomic fallback for `target_os=none`**
- [x] **Step 4: Re-run targeted tests and confirm GREEN**

## Chunk 3: Regression Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase70-rust-errno-thread-local-alignment.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
