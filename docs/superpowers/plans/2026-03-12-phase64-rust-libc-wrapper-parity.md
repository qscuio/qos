# Phase 64 Rust libc Wrapper Parity Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring Rust libc API surface to feature parity with the expanded C libc wrappers for memory/stdio/file/process/network/signal coverage.

**Architecture:** Add host-backed FFI wrappers in `qos-libc` (with `target_os=none` fallbacks) and validate with Rust unit tests that assert symbol presence and smoke basic safe flows.

**Tech Stack:** Rust `no_std` + host FFI, cargo test, pytest, cargo check, QEMU smoke via make

---

## Chunk 1: TDD Red

### Task 1: Add failing Rust tests for missing wrappers

**Files:**
- Modify: `rust-os/libc/tests/basic.rs`

- [x] **Step 1: Add imports/references for missing wrapper functions**
- [x] **Step 2: Add host smoke checks (malloc/realloc/free, strtok, socket/close, fopen/fwrite/fclose)**
- [x] **Step 3: Run `cargo test -q -p qos-libc` and confirm RED due unresolved wrappers**

## Chunk 2: Wrapper Implementation

### Task 2: Implement Rust wrapper surface

**Files:**
- Modify: `rust-os/libc/src/lib.rs`

- [x] **Step 1: Add host `extern \"C\"` declarations and `target_os=none` fallbacks**
- [x] **Step 2: Implement wrappers for memory/file/process/network/signal families**
- [x] **Step 3: Wire `qos_getpid` to host `getpid` on host targets**
- [x] **Step 4: Add remaining wrappers (`qos_printf`, `qos_snprintf`, `qos_vsnprintf`, `qos_perror`)**
- [x] **Step 5: Re-run `cargo test -q -p qos-libc` and confirm GREEN**

## Chunk 3: Regression Verification

### Task 3: End-to-end checks after parity expansion

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase64-rust-libc-wrapper-parity.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
