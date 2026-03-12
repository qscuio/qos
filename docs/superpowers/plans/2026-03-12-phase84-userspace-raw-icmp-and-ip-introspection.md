# Phase 84 Userspace Raw ICMP and IP Introspection Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enable a general userspace-usable raw ICMP socket path (for `ping`) and basic `ip` introspection commands aligned with the design doc networking/userspace direction.

**Architecture:** Extend C and Rust syscall socket state to support `SOCK_RAW` alongside stream/dgram without changing existing syscall numbers. Drive behavior through tests first, then route userspace `ping` through syscall socket APIs (instead of direct kernel net helpers), and add deterministic `ip addr`/`ip route` output in both shells.

**Tech Stack:** C11 kernel/userspace, Rust kernel/userspace, pytest, cargo test

---

### Task 1: Add RED tests for raw socket syscall behavior in C and Rust

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add C test assertions for `SOCK_RAW` creation and ICMP echo send/recv via syscall `socket/connect/send/recv`.**
- [x] **Step 2: Add Rust test assertions mirroring C raw socket behavior and unreachable host failure path.**
- [x] **Step 3: Run targeted tests and confirm RED.**

### Task 2: Implement `SOCK_RAW` in C and Rust syscall layers

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add `SOCK_RAW` socket type constant and allow it in syscall `socket()` validation.**
- [x] **Step 2: Add per-fd raw reply buffering/state fields and reset handling.**
- [x] **Step 3: Implement raw `connect/send/recv` path using existing ICMP echo primitive (`qos_net_icmp_echo`/`net_icmp_echo`).**
- [x] **Step 4: Preserve existing stream/dgram behavior and backlog/port allocation semantics.**

### Task 3: Route shell `ping` to socket syscall API and add `ip` command

**Files:**
- Modify: `c-os/userspace/shell.c`
- Modify: `rust-os/userspace/src/main.rs`
- Modify: `tests/test_userspace_shell_contracts.py`

- [x] **Step 1: Update C shell `ping` to prefer syscall socket raw-ICMP path (with existing deterministic fallback when kernel symbols are unavailable).**
- [x] **Step 2: Update Rust userspace `ping` to use syscall socket path instead of direct `net_icmp_echo` call.**
- [x] **Step 3: Add `ip` command support in both shells for `ip addr` and `ip route` outputs consistent with spec static network configuration.**
- [x] **Step 4: Extend shell contract tests for `ip addr`/`ip route` and ensure `ping` behavior remains correct.**

### Task 4: Verification sweep

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase84-userspace-raw-icmp-and-ip-introspection.md`

- [x] **Step 1: Run targeted tests for kernel syscall core and userspace shell contracts.**
- [x] **Step 2: Run `pytest -q` and `cd rust-os && cargo test -q -p qos-kernel --test syscall_core_remaining` for regression confidence.**
- [x] **Step 3: Update plan checkboxes to reflect executed steps and record verification outputs.**

## Verification Notes

- `pytest -q tests/test_kernel_syscall_core_remaining.py` -> pass (`5 passed`)
- `cd rust-os && cargo test -q -p qos-kernel --test syscall_core_remaining` -> pass (`5 passed`)
- `pytest -q tests/test_userspace_shell_contracts.py` -> pass (`3 passed`)
- `cd rust-os && cargo test -q -p qos-kernel` -> pass
- `cd rust-os && cargo test -q -p qos-libc` -> pass
- `pytest -q` -> pass (`97 passed`)
