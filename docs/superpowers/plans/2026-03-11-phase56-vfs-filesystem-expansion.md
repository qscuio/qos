# Phase 56 VFS Filesystem Expansion Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand VFS and syscall behavior toward spec split semantics across initramfs/tmpfs/procfs/ext2 namespaces while preserving prior contracts.

**Architecture:** Add mount-kind classification in VFS (`/tmp`, `/proc`, `/data`, default root), then integrate syscall-aware procfs virtual files (`/proc/meminfo`, `/proc/<pid>/status`) as read-only synthetic descriptors. Keep existing path-backed file flows unchanged for writable paths.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test

---

## Chunk 1: TDD Contract

### Task 1: Add failing split-behavior tests

**Files:**
- Create: `tests/test_kernel_vfs_split.py`
- Create: `rust-os/kernel/tests/vfs_split.rs`

- [x] **Step 1: Add C tests for VFS mount-kind classification and procfs syscall behavior**
- [x] **Step 2: Add Rust tests for VFS mount-kind classification and procfs syscall behavior**
- [x] **Step 3: Run `pytest -q tests/test_kernel_vfs_split.py` and confirm red**
- [x] **Step 4: Run `cargo test -q -p qos-kernel --test vfs_split` and confirm red**

## Chunk 2: Implementation

### Task 2: Implement mount split and procfs syscall wiring

**Files:**
- Modify: `c-os/kernel/fs/vfs.h`
- Modify: `c-os/kernel/fs/vfs.c`
- Modify: `c-os/kernel/syscall/syscall.c`
- Modify: `rust-os/kernel/src/lib.rs`

- [x] **Step 1: Add VFS mount kind constants and classification APIs in C and Rust**
- [x] **Step 2: Add proc-path parsing + synthetic render helpers in C and Rust syscall layers**
- [x] **Step 3: Add PROC FD metadata and lifecycle handling (`alloc`, `clear`, `dup2`)**
- [x] **Step 4: Wire `open/read/write/lseek/stat` for procfs virtual files**
- [x] **Step 5: Enforce procfs read-only behavior for `mkdir`/`unlink`**

## Chunk 3: Verification

### Task 3: Validate phase behavior

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase56-vfs-filesystem-expansion.md`

- [x] **Step 1: Run `pytest -q tests/test_kernel_vfs_split.py`**
- [x] **Step 2: Run `cargo test -q -p qos-kernel --test vfs_split`**
