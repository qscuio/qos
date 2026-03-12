# Phase 60 Libc String Expansion Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve C/Rust libc feature alignment with design requirements by adding missing string/parse helpers.

**Architecture:** Add matching helpers in both implementations and validate via shared contract tests. Keep APIs minimal and deterministic for no-std/test harness use.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing helper coverage

**Files:**
- Modify: `tests/test_pending_functions.py`
- Modify: `rust-os/libc/tests/basic.rs`

- [x] **Step 1: Add C symbol/behavior tests for `qos_strcpy`, `qos_strncpy`, `qos_strchr`, `qos_strrchr`, `qos_atoi`**
- [x] **Step 2: Add Rust unit coverage for corresponding helpers**
- [x] **Step 3: Run `pytest -q tests/test_pending_functions.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-libc` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement helpers in C and Rust

**Files:**
- Modify: `c-os/libc/libc.c`
- Modify: `rust-os/libc/src/lib.rs`

- [x] **Step 1: Implement C helpers (`strcpy`, `strncpy`, `strchr`, `strrchr`, `atoi`)**
- [x] **Step 2: Implement Rust helpers with analogous behavior**
- [x] **Step 3: Fix test expectation mismatch for last-occurrence index in `"qos-shell"`**

## Chunk 3: Verification

### Task 3: Validate phase behavior

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase60-libc-string-expansion.md`

- [x] **Step 1: Run `pytest -q tests/test_pending_functions.py`**
- [x] **Step 2: Run `cargo test -q -p qos-libc`**
