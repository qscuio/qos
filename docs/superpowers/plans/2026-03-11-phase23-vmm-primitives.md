# Phase 23 VMM Primitive Mapping Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement minimal virtual memory manager (VMM) primitives for map/unmap/translate semantics in both C and Rust kernels.

**Architecture:** Add fixed-capacity VA->PA mapping tables with page-alignment checks, remap semantics for duplicate VA entries, translation with page-offset support, and per-entry flags.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing VMM behavior tests

**Files:**
- Create: `tests/test_kernel_vmm.py`
- Create: `rust-os/kernel/tests/vmm.rs`

- [x] **Step 1: Add C tests for map/translate/unmap, unaligned rejection, and remap behavior**
- [x] **Step 2: Add Rust tests for map/translate/unmap, unaligned rejection, and remap behavior**
- [x] **Step 3: Run `pytest -q tests/test_kernel_vmm.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement VMM primitives in C and Rust

**Files:**
- Modify: `c-os/kernel/mm/mm.h`
- Modify: `c-os/kernel/mm/mm.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C VMM constants/APIs and map table implementation**
- [x] **Step 2: Add Rust VMM constants/APIs and map table implementation**
- [x] **Step 3: Ensure translation includes page-offset handling**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase23-vmm-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_vmm.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
