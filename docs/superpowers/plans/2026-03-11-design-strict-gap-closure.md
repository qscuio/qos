# Design Strict Gap Closure Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close all remaining gaps so implementation strictly follows `docs/superpowers/specs/2026-03-11-qos-design.md` across both C and Rust variants.

**Architecture:** Deliver in vertical slices with parity across languages. Each slice introduces tests first, implements minimal compliant behavior, then runs full verification.

**Tech Stack:** C11, Rust `no_std`, pytest, cargo test, QEMU smoke

---

## Remaining Slices

- [x] **Phase 53: Network core primitives (ARP/IPv4 route/UDP demux/TCP FSM)**
- [x] **Phase 54: Socket syscall integration with sockaddr parsing and protocol-aware flows (TCP/UDP)**
- [x] **Phase 55: Driver model enrichment (e1000/virtio-net descriptors, IRQ metadata, ring bookkeeping)**
- [x] **Phase 56: VFS/filesystem expansion toward initramfs/tmpfs/procfs/ext2 split behavior in spec**
- [x] **Phase 57: Userspace shell/program feature alignment (`ls/cat/echo/mkdir/rm/ps/ping/wget` behavior contracts)**
- [x] **Phase 58: Boot/arch boundary tightening for x86_64 + aarch64 requirements called out by spec**
- [x] **Phase 59: Final spec-conformance audit and regression hardening**
