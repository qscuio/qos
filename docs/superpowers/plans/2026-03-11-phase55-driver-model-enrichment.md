# Phase 55 Driver Model Enrichment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enrich driver subsystem with NIC descriptor metadata and ring bookkeeping aligned with spec-level driver requirements.

**Architecture:** Extend driver registries in C and Rust with optional NIC descriptors (`mmio_base`, IRQ, RX/TX ring sizes and head/tail indices). Keep existing generic name-based registration APIs and add NIC-specific registration/read/update APIs.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing NIC metadata tests

**Files:**
- Modify: `tests/test_kernel_drivers.py`
- Modify: `rust-os/kernel/tests/drivers.rs`

- [x] **Step 1: Add C tests for NIC registration, descriptor retrieval, and ring advancement**
- [x] **Step 2: Add Rust tests for NIC registration, descriptor retrieval, and ring advancement**
- [x] **Step 3: Run `pytest -q tests/test_kernel_drivers.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel --test drivers` and confirm red**

## Chunk 2: Implementation

### Task 2: Add NIC descriptor APIs in C and Rust

**Files:**
- Modify: `c-os/kernel/drivers/drivers.h`
- Modify: `c-os/kernel/drivers/drivers.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add C NIC descriptor type and API declarations**
- [x] **Step 2: Add C NIC descriptor storage and reset wiring**
- [x] **Step 3: Implement C NIC register/get/update (`advance_tx`, `consume_rx`)**
- [x] **Step 4: Extend Rust driver state with NIC descriptor fields**
- [x] **Step 5: Add Rust `DriverNicDesc` public struct**
- [x] **Step 6: Implement Rust NIC register/get/update APIs**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase55-driver-model-enrichment.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_drivers.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel --test drivers`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo test -q -p qos-kernel`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
