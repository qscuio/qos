# Phase 22 PMM From BootInfo Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a minimal physical memory manager (PMM) initialized from `boot_info` usable ranges, with deterministic page allocation/free behavior in both C and Rust kernels.

**Architecture:** Build fixed-capacity frame tables from `mmap_entries` (type=usable), reserve known regions (low memory + initramfs), expose allocator stats and alloc/free APIs, and integrate PMM init into kernel entry.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing PMM behavior tests

**Files:**
- Create: `tests/test_kernel_pmm.py`
- Create: `rust-os/kernel/tests/pmm.rs`

- [x] **Step 1: Add C PMM tests for page counting and alloc/free round-trip**
- [x] **Step 2: Add Rust PMM tests for page counting, alloc/free, and invalid boot info**
- [x] **Step 3: Run `pytest -q tests/test_kernel_pmm.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement PMM init/allocation in C and Rust

**Files:**
- Create: `c-os/kernel/mm/mm.h`
- Modify: `c-os/kernel/mm/mm.c`
- Modify: `c-os/kernel/kernel.h`
- Modify: `c-os/kernel/kernel.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Implement C PMM frame table, reserve logic, and alloc/free APIs**
- [x] **Step 2: Integrate C PMM initialization into `qos_kernel_entry`**
- [x] **Step 3: Implement Rust PMM frame table, reserve logic, and alloc/free APIs**
- [x] **Step 4: Integrate Rust PMM initialization into `kernel_entry`**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase22-pmm-from-bootinfo.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_pmm.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
