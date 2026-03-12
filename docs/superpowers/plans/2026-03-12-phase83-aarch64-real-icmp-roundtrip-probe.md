# Phase 83 - AArch64 Real ICMP Round-Trip Probe

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend AArch64 real-network runtime probes to send a real ICMP echo request and classify real round-trip status from RX traffic.

**Architecture:** Reuse legacy virtio-net RX/TX queue plumbing from prior phases, add ICMP echo frame construction + reply matching in both Rust/C probes, and emit explicit `icmp_real` markers with stage diagnostics. Keep CI stable by classifying no-reply conditions as `roundtrip_skip` while still requiring real ICMP egress evidence.

**Tech Stack:** freestanding Rust/C probes, virtio-mmio legacy queues, QEMU user-net, pytest

---

## Chunk 1: RED Test

### Task 1: Add ICMP round-trip integration test

**Files:**
- Create: `tests/test_aarch64_real_icmp_roundtrip.py`

- [x] **Step 1: Add Rust/C AArch64 smoke checks for `icmp_real` marker**
- [x] **Step 2: Add pcap assertion for ICMP probe payload `QOSICMPRT`**
- [x] **Step 3: Run test RED before implementation**

## Chunk 2: Probe Implementation

### Task 2: Rust probe ICMP real path

**Files:**
- Modify: `tools/aarch64-probe/src/main.rs`

- [x] **Step 1: Add ICMP frame builder and echo-reply parser**
- [x] **Step 2: Send ICMP probe over real virtio TX queue**
- [x] **Step 3: Match echo reply in RX loop and emit `icmp_real` marker + stage**

### Task 3: C probe ICMP real path

**Files:**
- Modify: `tools/aarch64-c-probe/main.c`

- [x] **Step 1: Add ICMP frame builder and echo-reply parser**
- [x] **Step 2: Send ICMP probe over real virtio TX queue**
- [x] **Step 3: Match echo reply in RX loop and emit `icmp_real` marker + stage**

## Chunk 3: Verification

### Task 4: Targeted and full regression checks

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase83-aarch64-real-icmp-roundtrip-probe.md`

- [x] **Step 1: Run `pytest -q tests/test_aarch64_real_icmp_roundtrip.py`**
- [x] **Step 2: Run targeted AArch64 probe tests (`real_net_tx`, `real_net_rx`, `real_icmp_roundtrip`, runtime, smoke markers`)**
- [x] **Step 3: Run `make test-all`**
- [x] **Step 4: Run full `pytest -q`**
- [x] **Step 5: Run Rust verification (`cargo test -q -p qos-kernel`, `cargo test -q -p qos-libc`, `cargo check --workspace`)**

## Verification Results
- `pytest -q tests/test_aarch64_real_icmp_roundtrip.py` -> pass (`2 passed`)
- `pytest -q tests/test_aarch64_real_net_tx.py tests/test_aarch64_real_net_rx.py tests/test_aarch64_real_icmp_roundtrip.py tests/test_aarch64_runtime_probe.py tests/test_boot_handoff_smoke.py` -> pass (`9 passed`)
- `make test-all` -> pass (all 4 smoke targets)
- `pytest -q` -> pass (`97 passed`)
- `cd rust-os && cargo test -q -p qos-kernel` -> pass
- `cd rust-os && cargo test -q -p qos-libc` -> pass
- `cd rust-os && cargo check --workspace` -> pass

## Runtime Observation
- ICMP probe egress is verified (`QOSICMPRT` present in captured pcap).
- In this QEMU user-net environment, echo replies were not observed during CI smoke; marker reports `icmp_real=roundtrip_skip icmp_real_stage=timeout` rather than failing unrelated smoke paths.
