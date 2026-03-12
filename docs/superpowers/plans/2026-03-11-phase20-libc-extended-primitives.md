# Phase 20 Libc Extended Primitive Functions Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend C/Rust libc helper coverage beyond the initial stub-replacement set by implementing additional core primitives (`strncmp`, `memcmp`, `memmove`) with test coverage.

**Architecture:** Add pure, dependency-free implementations in C and Rust (`no_std` compatible for Rust libc), then expand existing tests to validate ordering semantics and overlapping move behavior.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test, gcc

---

## Chunk 1: TDD Contract

### Task 1: Add failing tests for new primitives

**Files:**
- Modify: `tests/test_pending_functions.py`
- Modify: `rust-os/libc/tests/basic.rs`

- [x] **Step 1: Add C test expectations for `qos_strncmp`, `qos_memcmp`, `qos_memmove`**
- [x] **Step 2: Add Rust libc test expectations for `qos_strncmp`, `qos_memcmp`, `qos_memmove`**
- [x] **Step 3: Run `pytest -q tests/test_pending_functions.py` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement extended primitives in C and Rust libc

**Files:**
- Modify: `c-os/libc/libc.c`
- Modify: `rust-os/libc/src/lib.rs`

- [x] **Step 1: Implement `qos_strncmp`**
- [x] **Step 2: Implement `qos_memcmp`**
- [x] **Step 3: Implement `qos_memmove` with overlap-safe behavior**

## Chunk 3: Verification

### Task 3: End-to-end regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase20-libc-extended-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_pending_functions.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 4: Run `make test-all`**
