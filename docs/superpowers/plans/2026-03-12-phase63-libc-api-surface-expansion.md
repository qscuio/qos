# Phase 63 libc API Surface Expansion Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand C libc exported wrapper coverage to better match the design-doc API surface for memory, stdio/file, process, network, and signal families.

**Architecture:** Add thin `qos_*` wrapper functions over host libc/POSIX APIs, then validate export presence plus safe smoke behavior with pytest.

**Tech Stack:** gcc, ctypes, pytest

---

## Chunk 1: API Surface Implementation

### Task 1: Add missing C wrapper exports

**Files:**
- Modify: `c-os/libc/libc.c`

- [x] **Step 1: Add POSIX feature-test defines for signal/file descriptors under `-std=c11`**
- [x] **Step 2: Add wrappers for memory + tokenization (`qos_malloc/free/realloc/strtok`)**
- [x] **Step 3: Add wrappers for stdio/file APIs (`qos_printf`, `qos_fopen` family, `qos_fileno`)**
- [x] **Step 4: Add wrappers for process/network/signal APIs (`qos_fork/exec*/waitpid`, `qos_socket*`, `qos_sig*`)**

## Chunk 2: TDD/Verification

### Task 2: Validate new surface and behavior

**Files:**
- Modify: `tests/test_pending_functions.py`

- [x] **Step 1: Add symbol-coverage assertions for newly exported wrappers**
- [x] **Step 2: Add safe behavior checks for malloc/realloc/free, strtok, socket/close, and fopen/fwrite/fclose**
- [x] **Step 3: Run `pytest -q tests/test_pending_functions.py` and confirm passing**

## Chunk 3: Full Regression Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase63-libc-api-surface-expansion.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
