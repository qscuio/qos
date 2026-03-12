# Phase 54 Syscall Socket Protocol Integration Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make socket syscalls protocol-aware and sockaddr-driven per design intent.

**Architecture:** Extend syscall fd socket metadata to track socket type and endpoint ports/IP. Parse sockaddr in bind/connect, reject invalid addr lengths, route datagram send/recv through UDP demux APIs, and keep stream behavior through TCP/listener logic + existing queue bridge.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing syscall protocol tests

**Files:**
- Modify: `tests/test_kernel_syscall_core_remaining.py`
- Modify: `rust-os/kernel/tests/syscall_core_remaining.rs`

- [x] **Step 1: Add sockaddr-populated socket tests for stream and dgram**
- [x] **Step 2: Add failing assertions for invalid bind addrlen and dgram listen rejection**
- [x] **Step 3: Add failing assertion that UDP connect works without active TCP listener**
- [x] **Step 4: Add failing assertion for UDP demux (wrong destination socket recv == -1)**
- [x] **Step 5: Run `pytest -q tests/test_kernel_syscall_core_remaining.py` and confirm red**
- [x] **Step 6: Run `cargo test -q -p qos-kernel --test syscall_core_remaining` and confirm red**

## Chunk 2: Implementation

### Task 2: Protocol-aware socket syscall behavior

**Files:**
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add socket metadata fields (type/local port/remote port/remote IP/conn id) to fd state**
- [x] **Step 2: Implement sockaddr parser and enforce minimum length/valid port+ip**
- [x] **Step 3: Require `AF_INET` + valid socket type at `socket()`**
- [x] **Step 4: Make `bind()` parse sockaddr and validate dgram bind through UDP layer**
- [x] **Step 5: Make `listen()` stream-only and register listener with TCP layer**
- [x] **Step 6: Make `connect()` stream path listener/port-aware and dgram path listener-independent**
- [x] **Step 7: Route dgram `send/recv` and socket `write/read` to UDP APIs**
- [x] **Step 8: Preserve stream send/recv behavior and pending accept model compatibility**
- [x] **Step 9: Keep socket metadata consistent across `dup2` and `close` reset paths**

## Chunk 3: Verification

### Task 3: End-to-end checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase54-syscall-socket-protocol-integration.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_syscall_core_remaining.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel --test syscall_core_remaining`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run `cargo test -q -p qos-kernel`**
- [x] **Step 5: Run `cargo check --workspace`**
- [x] **Step 6: Run `make test-all`**
