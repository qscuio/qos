# Phase 80 - AArch64 Real Virtio-Net TX Proof

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the remaining modeled AArch64 Rust network proof with a real guest-to-netdev packet transmission path via virtio-mmio.

**Architecture:** Keep kernel init + modeled ICMP checks for continuity, then add an explicit virtio-net TX probe in the Rust AArch64 runtime probe. Capture packets through QEMU `filter-dump` and assert a probe payload appears in pcap, proving actual guest packet egress.

**Tech Stack:** Rust `no_std` probe, virtio-mmio split queue setup (legacy v1 path), QEMU netdev capture, pytest

---

## Chunk 1: RED Test for Real TX Evidence

### Task 1: Add failing integration test

**Files:**
- Create: `tests/test_aarch64_real_net_tx.py`

- [x] **Step 1: Add test requiring Rust AArch64 smoke log marker `net_tx=real_ok`**
- [x] **Step 2: Add test requiring pcap capture file to contain payload marker `QOSREALNET`**
- [x] **Step 3: Run `pytest -q tests/test_aarch64_real_net_tx.py` and confirm RED**

## Chunk 2: QEMU Capture Plumbing + Probe TX Implementation

### Task 2: Add capture hook in AArch64 launcher

**Files:**
- Modify: `qemu/aarch64.sh`

- [x] **Step 1: Add optional `QOS_PCAP_PATH` handling**
- [x] **Step 2: Attach `-object filter-dump,id=f0,netdev=net0,file=<path>` when set**

### Task 3: Implement real virtio TX probe in Rust AArch64 runtime path

**Files:**
- Modify: `tools/aarch64-probe/src/main.rs`

- [x] **Step 1: Add virtio-mmio discovery over DTB-derived MMIO slot range**
- [x] **Step 2: Implement legacy (`version=1`) virtqueue TX setup and notify path**
- [x] **Step 3: Emit one Ethernet probe frame carrying payload `QOSREALNET`**
- [x] **Step 4: Poll used ring for TX completion and expose `net_tx=real_ok|real_fail` marker**
- [x] **Step 5: Keep existing boot + ICMP markers intact and append net TX status fields**

## Chunk 3: Contract Hardening + Regression Verification

### Task 4: Extend smoke contract and verify full matrix

**Files:**
- Modify: `tests/test_boot_handoff_smoke.py`

- [x] **Step 1: Require `net_tx=real_ok` in Rust AArch64 smoke marker set**
- [x] **Step 2: Run targeted smoke tests (`test_aarch64_real_net_tx`, `test_aarch64_runtime_probe`, `test_boot_handoff_smoke`)**
- [x] **Step 3: Run full verification (`make test-all`, `pytest -q`, rust kernel/libc tests, workspace check)**

## Verification Results
- `pytest -q tests/test_aarch64_real_net_tx.py` -> pass
- `pytest -q tests/test_aarch64_real_net_tx.py tests/test_aarch64_runtime_probe.py tests/test_boot_handoff_smoke.py` -> pass
- `make test-all` -> pass (all 4 smoke targets)
- `pytest -q` -> pass (`92 passed`)
- `cd rust-os && cargo test -q -p qos-kernel` -> pass
- `cd rust-os && cargo test -q -p qos-libc` -> pass
- `cd rust-os && cargo check --workspace` -> pass
