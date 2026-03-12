# Phase 57 Userspace Feature Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align userspace shell/program behavior contracts with spec-level command coverage (`ls/cat/echo/mkdir/rm/ps/ping/wget`) for both C and Rust implementations.

**Architecture:** Expand shell command dispatchers into lightweight command suites with deterministic outputs and in-process path bookkeeping. Preserve existing prompt/help/exit behavior and Rust `--once` flow.

**Tech Stack:** C11 userspace binary, Rust std userspace binary, pytest

---

## Chunk 1: TDD Contract

### Task 1: Add failing userspace contract tests

**Files:**
- Create: `tests/test_userspace_shell_contracts.py`

- [x] **Step 1: Add C shell contract test for command coverage and expected outputs**
- [x] **Step 2: Add Rust userspace `--once` contract checks for same command set**
- [x] **Step 3: Run `pytest -q tests/test_userspace_shell_contracts.py` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement command suite in C and Rust userspace

**Files:**
- Modify: `c-os/userspace/shell.c`
- Modify: `rust-os/userspace/src/main.rs`

- [x] **Step 1: Add shared command coverage (`ls`, `cat`, `echo`, `mkdir`, `rm`, `ps`, `ping`, `wget`)**
- [x] **Step 2: Add minimal in-process path registry for `mkdir/rm/ls`**
- [x] **Step 3: Add proc-style outputs for `cat /proc/meminfo` and `/proc/<pid>/status`**
- [x] **Step 4: Preserve existing `help`, `exit`, prompt loop, and Rust `--once` support**

## Chunk 3: Verification

### Task 3: Validate phase behavior

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase57-userspace-feature-alignment.md`

- [x] **Step 1: Run `pytest -q tests/test_userspace_shell_contracts.py`**
