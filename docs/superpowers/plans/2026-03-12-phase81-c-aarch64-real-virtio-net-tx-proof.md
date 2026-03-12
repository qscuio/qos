# Phase 81 - C AArch64 Real Virtio-Net TX Proof

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring C AArch64 runtime smoke behavior to parity with Rust by proving real guest packet egress through virtio-mmio TX.

**Architecture:** Extend the C AArch64 probe with legacy virtio-net TX queue setup and one UDP/IPv4 probe packet (`QOSREALNET`) to QEMU slirp gateway. Require `net_tx=real_ok` marker in C smoke contract and verify both pcap capture and host UDP receive.

**Tech Stack:** freestanding C probe, virtio-mmio legacy queue, QEMU filter-dump, pytest

---

## Chunk 1: RED Contract and Integration Test

### Task 1: Add failing C parity checks

**Files:**
- Modify: `tests/test_aarch64_real_net_tx.py`
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Add C AArch64 integration test requiring `net_tx=real_ok` marker**
- [x] **Step 2: Add C AArch64 integration test requiring pcap payload `QOSREALNET` and host UDP receive**
- [x] **Step 3: Tighten smoke marker contract to require `net_tx=real_ok` for C AArch64**
- [x] **Step 4: Run targeted tests and confirm RED before implementation**

## Chunk 2: C Probe Real TX Implementation

### Task 2: Implement C virtio-mmio TX probe path

**Files:**
- Modify: `tools/aarch64-c-probe/main.c`

- [x] **Step 1: Add virtio-mmio discovery for net device and version capture**
- [x] **Step 2: Add legacy TX queue initialization using `QUEUE_PFN` path**
- [x] **Step 3: Build Ethernet + IPv4 + UDP probe frame with payload `QOSREALNET`**
- [x] **Step 4: Submit TX descriptor and poll used ring completion**
- [x] **Step 5: Emit `net_tx=real_ok|real_fail` plus stage/version diagnostics in smoke marker**

## Chunk 3: Verification

### Task 3: Validate targeted and full regression

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase81-c-aarch64-real-virtio-net-tx-proof.md`

- [x] **Step 1: Run targeted tests (`test_aarch64_real_net_tx`, `test_aarch64_runtime_probe`, `test_boot_handoff_smoke`)**
- [x] **Step 2: Run `make test-all`**
- [x] **Step 3: Run `pytest -q`**
- [x] **Step 4: Run Rust verification (`cargo test -q -p qos-kernel`, `cargo test -q -p qos-libc`, `cargo check --workspace`)**

## Verification Results
- `pytest -q tests/test_aarch64_real_net_tx.py::test_c_aarch64_smoke_emits_real_virtio_tx_frame tests/test_boot_handoff_smoke.py::test_smoke_logs_include_stage_and_handoff_markers` -> pass
- `pytest -q tests/test_aarch64_real_net_tx.py tests/test_aarch64_runtime_probe.py tests/test_boot_handoff_smoke.py` -> pass (`5 passed`)
- `make test-all` -> pass (all 4 smoke targets)
- `pytest -q` -> pass (`93 passed`)
- `cd rust-os && cargo test -q -p qos-kernel` -> pass
- `cd rust-os && cargo test -q -p qos-libc` -> pass
- `cd rust-os && cargo check --workspace` -> pass
