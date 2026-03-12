# Phase 67 Shell CWD Prompt Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align shell prompt behavior with spec by displaying current directory in both C and Rust interactive shells.

**Architecture:** Render prompt with current working directory (`cwd`) each loop iteration and verify through userspace shell feature tests.

**Tech Stack:** C11 shell, Rust std userspace shell, pytest

---

## Chunk 1: TDD

### Task 1: Add prompt assertions

**Files:**
- Modify: `tests/test_userspace_shell_features.py`

- [x] **Step 1: Assert root prompt includes cwd (`qos-sh:/>`)**
- [x] **Step 2: Assert post-`cd` prompt includes updated cwd (`qos-sh:/tmp/work>`)**
- [x] **Step 3: Run targeted shell tests and confirm behavior**

## Chunk 2: Implementation

### Task 2: Render cwd in prompt

**Files:**
- Modify: `c-os/userspace/shell.c`
- Modify: `rust-os/userspace/src/main.rs`

- [x] **Step 1: Update C shell prompt formatting to include `g_cwd`**
- [x] **Step 2: Update Rust shell prompt formatting to include `state.cwd`**
- [x] **Step 3: Re-run shell feature/contract tests**

## Chunk 3: Regression Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase67-shell-cwd-prompt-alignment.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
