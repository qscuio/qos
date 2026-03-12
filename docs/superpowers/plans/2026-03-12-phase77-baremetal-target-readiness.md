# Phase 77 Bare-Metal Target Readiness Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Unblock cross-target kernel builds needed for eventual real QEMU runtime integration by making kernel code portable across `c_char` signedness and installing missing Rust bare-metal targets.

**Architecture:** Install `aarch64-unknown-none` and `x86_64-unknown-none` std components; replace target-specific raw pointer casts in `CStr::from_ptr` with `core::ffi::c_char`; verify host and bare-metal target builds plus full test suite.

**Tech Stack:** rustup, Rust kernel crate, pytest, cargo test, cargo check

---

## Chunk 1: Target Prerequisites

### Task 1: Install missing Rust bare-metal targets

**Files:**
- Modify: environment toolchain state

- [x] **Step 1: Run `rustup target add aarch64-unknown-none x86_64-unknown-none`**

## Chunk 2: Implementation

### Task 2: Fix cross-target `CStr::from_ptr` pointer casts

**Files:**
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Replace `*const i8`/`*const u8` pointer casts with `*const core::ffi::c_char` for `CStr::from_ptr` call sites**

## Chunk 3: Verification

### Task 3: Verify host and bare-metal target health

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase77-baremetal-target-readiness.md`

- [x] **Step 1: Run `cd rust-os && cargo check -p qos-kernel --target aarch64-unknown-none`**
- [x] **Step 2: Run `cd rust-os && cargo check -p qos-kernel --target x86_64-unknown-none`**
- [x] **Step 3: Run `cd rust-os && cargo check -p qos-kernel`**
- [x] **Step 4: Run `pytest -q`**
- [x] **Step 5: Run `cd rust-os && cargo test -q -p qos-kernel`**
- [x] **Step 6: Run `cd rust-os && cargo check --workspace`**
