# Phase 65 libc API Surface Guards Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Lock C/Rust libc API parity into regression tests so future changes cannot silently drop required design-doc functions.

**Architecture:** Add a repository-level pytest guard that checks required `qos_*` function names in both libc implementations, then run full project verification.

**Tech Stack:** pytest, cargo test, cargo check, make test-all

---

## Chunk 1: Guard Test

### Task 1: Add libc API parity guard

**Files:**
- Create: `tests/test_libc_api_surface.py`

- [x] **Step 1: Define required API list across memory/string/io/file/process/network/signal/error families**
- [x] **Step 2: Assert C libc source contains every required symbol name**
- [x] **Step 3: Assert Rust libc source contains every required symbol name**

## Chunk 2: Verification

### Task 2: Validate no regressions

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase65-libc-api-surface-guards.md`

- [x] **Step 1: Run `pytest -q tests/test_libc_api_surface.py tests/test_pending_functions.py`**
- [x] **Step 2: Run `cargo test -q -p qos-libc`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo test -q -p qos-kernel`**
- [x] **Step 5: Run `cargo test -q -p qos-libc`**
- [x] **Step 6: Run `cargo check --workspace`**
- [x] **Step 7: Run `make test-all`**
