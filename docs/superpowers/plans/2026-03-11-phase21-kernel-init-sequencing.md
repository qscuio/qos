# Phase 21 Kernel Init Sequencing Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace remaining kernel no-op init functions with deterministic initialization behavior and boot-info validation for both C and Rust kernels.

**Architecture:** Introduce a simple init-state bitmask in C and Rust kernels. Validate core `boot_info` invariants before subsystem initialization and expose deterministic kernel-entry behavior for host-side tests.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing kernel behavior tests

**Files:**
- Create: `tests/test_kernel_init.py`
- Create: `rust-os/kernel/tests/basic.rs`

- [x] **Step 1: Add C test expectations for `qos_kernel_entry`, `qos_kernel_reset`, and init-state bitmask**
- [x] **Step 2: Add Rust kernel tests for boot-info validation and full init-state completion**
- [x] **Step 3: Run `pytest -q tests/test_kernel_init.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement C/Rust kernel init sequencing

**Files:**
- Create: `c-os/kernel/kernel.h`
- Create: `c-os/kernel/init_state.h`
- Create: `c-os/kernel/init_state.c`
- Create: `c-os/kernel/kernel.c`
- Modify: `c-os/kernel/mm/mm.c`
- Modify: `c-os/kernel/drivers/drivers.c`
- Modify: `c-os/kernel/fs/vfs.c`
- Modify: `c-os/kernel/net/net.c`
- Modify: `c-os/kernel/sched/sched.c`
- Modify: `rust-os/kernel/src/lib.rs`
- Modify: `tests/test_kernel_init.py`

- [x] **Step 1: Add shared init-state tracking for C kernel modules**
- [x] **Step 2: Implement `qos_boot_info_validate` and `qos_kernel_entry` in C**
- [x] **Step 3: Implement init-state constants, boot-info validation, and `kernel_entry` result flow in Rust kernel**
- [x] **Step 4: Wire C subsystem init functions to update deterministic state bits**

## Chunk 3: Verification

### Task 3: Regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase21-kernel-init-sequencing.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_init.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
