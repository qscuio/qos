# Phase 32 Process Signal State Primitive Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement per-process signal state primitives in C and Rust process tables, including mask/pending/handler operations and fork/exec signal semantics from the architecture spec.

**Architecture:** Extend process state with `sig_handlers[32]`, `sig_pending`, and `sig_blocked`; enforce SIGKILL/SIGSTOP uncatchable/unblockable rules; support fork inheritance (`handlers`, `blocked`) with cleared pending; support exec reset (clear pending, keep mask, reset handlers except `SIG_IGN`).

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing proc-signal tests

**Files:**
- Create: `tests/test_kernel_proc_signal.py`
- Create: `rust-os/kernel/tests/proc_signal.rs`

- [x] **Step 1: Add C tests for handler/mask/pending delivery and fork/exec semantics**
- [x] **Step 2: Add Rust tests for handler/mask/pending delivery and fork/exec semantics**
- [x] **Step 3: Run `pytest -q tests/test_kernel_proc_signal.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel proc_signal` in `rust-os/` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement proc-signal APIs in C and Rust

**Files:**
- Modify: `c-os/kernel/proc/proc.h`
- Modify: `c-os/kernel/proc/proc.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add signal constants and proc-signal API declarations in C**
- [x] **Step 2: Add C proc-signal state fields and implement APIs (`set/get handler`, `set/get mask`, `send/pending/next`)**
- [x] **Step 3: Add C fork and exec signal state semantics primitives**
- [x] **Step 4: Add Rust proc-signal state fields and matching APIs**
- [x] **Step 5: Add Rust fork and exec signal state semantics primitives**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase32-proc-signal-state-primitives.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_proc.py tests/test_kernel_proc_signal.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel` in `rust-os/`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo check --workspace` in `rust-os/`**
- [x] **Step 5: Run `make test-all`**
