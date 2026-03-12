# Phase 61 Shell Feature Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve shell behavior parity with spec direction: quoted tokenization, redirection, simple pipes, and core built-ins (`cd`, `pwd`, `export`, `unset`) in both C and Rust userspace shells.

**Architecture:** Replace whitespace-only parsing with tokenization that understands quotes and shell operators (`|`, `<`, `>`, `>>`). Keep the shell deterministic with in-process path/file/env state and lightweight command execution suitable for test harnesses.

**Tech Stack:** C11 userspace shell, Rust std userspace shell, pytest

---

## Chunk 1: TDD Contract

### Task 1: Add failing feature tests

**Files:**
- Create: `tests/test_userspace_shell_features.py`

- [x] **Step 1: Add C shell integration test for quotes, pipe, redirection, and built-ins**
- [x] **Step 2: Add Rust shell integration test for same contract**
- [x] **Step 3: Run `pytest -q tests/test_userspace_shell_features.py` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement shell parser/executor enhancements

**Files:**
- Modify: `c-os/userspace/shell.c`
- Modify: `rust-os/userspace/src/main.rs`

- [x] **Step 1: Add tokenizers handling quotes and shell operators**
- [x] **Step 2: Add one-stage pipe execution (`cmd1 | cmd2`)**
- [x] **Step 3: Add redirection handling (`<`, `>`, `>>`) via in-memory file map**
- [x] **Step 4: Add built-ins `cd`, `pwd`, `export`, `unset`**
- [x] **Step 5: Preserve existing command contracts (`ls/cat/echo/mkdir/rm/ps/ping/wget/help/exit`)**

## Chunk 3: Verification

### Task 3: Validate phase behavior

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase61-shell-feature-alignment.md`

- [x] **Step 1: Run `pytest -q tests/test_userspace_shell_features.py tests/test_userspace_shell_contracts.py tests/test_pending_functions.py`**
- [x] **Step 2: Run `pytest -q`**
- [x] **Step 3: Run `cargo test -q -p qos-kernel`**
- [x] **Step 4: Run `cargo test -q -p qos-libc`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
