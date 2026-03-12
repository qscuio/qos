# Phase 25 VFS Primitive Path Table Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal VFS primitives for create/exists/remove path management in both C and Rust kernels.

**Architecture:** Add fixed-capacity in-memory path tables with duplicate prevention and basic path validation (`/`-prefixed absolute paths). Keep `vfs_init` as subsystem marker while resetting VFS state for deterministic behavior.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing VFS behavior tests

**Files:**
- Create: `tests/test_kernel_vfs.py`
- Create: `rust-os/kernel/tests/vfs.rs`

- [x] **Step 1: Add C tests for create/exists/remove flow and invalid-path handling**
- [x] **Step 2: Add Rust tests for create/exists/remove flow and invalid-path handling**
- [x] **Step 3: Run `pytest -q tests/test_kernel_vfs.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement VFS primitives in C and Rust

**Files:**
- Create: `c-os/kernel/fs/vfs.h`
- Modify: `c-os/kernel/fs/vfs.c`
- Modify: `c-os/kernel/kernel.h`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C VFS path table APIs (`reset/create/exists/remove/count`)**
- [x] **Step 2: Wire C `vfs_init` to deterministic reset + init marker**
- [x] **Step 3: Implement Rust VFS path table APIs with synchronization**
- [x] **Step 4: Wire Rust `vfs_init` to deterministic reset + init marker**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase25-vfs-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_vfs.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
