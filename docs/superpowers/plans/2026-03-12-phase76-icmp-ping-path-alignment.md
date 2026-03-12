# Phase 76 ICMP Ping Path Alignment Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace unconditional shell ping success output with target-aware ICMP behavior and add explicit ICMP echo support in C/Rust kernel net modules.

**Architecture:** Add RED tests for kernel ICMP echo behavior and shell unreachable reporting; implement `net_icmp_echo` helpers in both kernel net stacks; update shell ping handlers to validate IPv4 input and report unreachable for unknown hosts.

**Tech Stack:** C kernel net/userspace shell, Rust kernel/userspace shell, pytest, cargo test, cargo check, make

---

## Chunk 1: TDD

### Task 1: Add failing ICMP + shell ping tests

**Files:**
- Modify: `tests/test_kernel_net.py`
- Modify: `rust-os/kernel/tests/net.rs`
- Modify: `tests/test_userspace_shell_contracts.py`

- [x] **Step 1: Add C kernel net test `test_c_net_icmp_echo_gateway_and_unreachable`**
- [x] **Step 2: Add Rust kernel net test `net_icmp_echo_gateway_and_unreachable`**
- [x] **Step 3: Add shell behavior test `test_shell_ping_reports_unreachable_for_unknown_host`**
- [x] **Step 4: Run targeted tests and confirm RED**

## Chunk 2: Implementation

### Task 2: Implement ICMP echo helpers and ping behavior

**Files:**
- Modify: `c-os/kernel/net/net.h`
- Modify: `c-os/kernel/net/net.c`
- Modify: `rust-os/kernel/src/lib.rs`
- Modify: `c-os/userspace/shell.c`
- Modify: `rust-os/userspace/Cargo.toml`
- Modify: `rust-os/userspace/src/main.rs`

- [x] **Step 1: Add C net API `qos_net_icmp_echo` and implementation**
- [x] **Step 2: Add Rust net API `net_icmp_echo` and implementation**
- [x] **Step 3: Update C shell ping to parse IPv4 and report unreachable when ICMP echo fails**
- [x] **Step 4: Update Rust userspace ping to parse IPv4 and use kernel ICMP helper**

## Chunk 3: Verification

### Task 3: Verify targeted and full regressions

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase76-icmp-ping-path-alignment.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_net.py::test_c_net_icmp_echo_gateway_and_unreachable`**
- [x] **Step 2: Run `cd rust-os && cargo test -q -p qos-kernel --test net net_icmp_echo_gateway_and_unreachable`**
- [x] **Step 3: Run `pytest -q tests/test_userspace_shell_contracts.py::test_shell_ping_reports_unreachable_for_unknown_host`**
- [x] **Step 4: Run `pytest -q tests/test_kernel_net.py`**
- [x] **Step 5: Run `cd rust-os && cargo test -q -p qos-kernel --test net`**
- [x] **Step 6: Run `pytest -q tests/test_userspace_shell_contracts.py tests/test_userspace_shell_features.py tests/test_pending_functions.py::test_c_shell_help_and_exit`**
- [x] **Step 7: Run `pytest -q`**
- [x] **Step 8: Run `cd rust-os && cargo test -q -p qos-kernel`**
- [x] **Step 9: Run `cd rust-os && cargo test -q -p qos-libc`**
- [x] **Step 10: Run `cd rust-os && cargo check --workspace`**
- [x] **Step 11: Run `make test-all`**
