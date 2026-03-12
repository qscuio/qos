# Phase 66 Shell PATH Resolution Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align C and Rust shell command resolution with spec-level PATH search behavior for non-builtin programs while preserving builtin semantics and existing command contracts.

**Architecture:** Classify builtin vs external commands, add virtual executable path tables (`/bin`, `/usr/bin`), resolve external commands via explicit absolute paths or `PATH` env lookup, and reject missing path resolutions as unknown commands.

**Tech Stack:** C11 shell, Rust std userspace shell, pytest, cargo

---

## Chunk 1: TDD

### Task 1: Add failing PATH behavior tests

**Files:**
- Modify: `tests/test_userspace_shell_features.py`

- [x] **Step 1: Add C shell test proving `PATH` gates external command lookup (`ls`)**
- [x] **Step 2: Add Rust shell test proving same PATH behavior**
- [x] **Step 3: Run targeted shell tests and confirm RED**

## Chunk 2: Implementation

### Task 2: Implement PATH-aware command resolution in both shells

**Files:**
- Modify: `c-os/userspace/shell.c`
- Modify: `rust-os/userspace/src/main.rs`

- [x] **Step 1: Seed default PATH (`/bin:/usr/bin`) in shell state**
- [x] **Step 2: Add builtin classification for true builtins only (`help/exit/echo/cd/pwd/export/unset`)**
- [x] **Step 3: Add virtual executable path resolution tables for external commands**
- [x] **Step 4: Resolve explicit absolute paths and `PATH` search candidates; emit unknown for misses**
- [x] **Step 5: Re-run targeted shell tests and confirm GREEN**

## Chunk 3: Regression Verification

### Task 3: End-to-end checks after shell resolution changes

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase66-shell-path-resolution-alignment.md`

- [x] **Step 1: Run `pytest -q`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel`**
- [x] **Step 3: Run `cargo test -q -p qos-libc`**
- [x] **Step 4: Run `cargo check --workspace`**
- [x] **Step 5: Run `make test-all`**
