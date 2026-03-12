# Phase 82 - AArch64 Real Virtio-Net RX Proof

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add real guest RX datapath proof on AArch64 runtime probes (Rust and C) with optional host UDP forwarding.

**Architecture:** Extend QEMU AArch64 launcher with optional UDP `hostfwd` configuration and enhance both probes to configure legacy virtio-net RX+TX queues, consume RX completions, and emit `net_rx` status markers. Keep default smoke behavior stable by mapping RX timeout to `real_skip`.

**Tech Stack:** freestanding Rust/C probes, virtio-mmio legacy split queue, QEMU user-net hostfwd, pytest

---

## Chunk 1: RED Test for Real RX Evidence

### Task 1: Add failing integration tests

**Files:**
- Create: `tests/test_aarch64_real_net_rx.py`

- [x] **Step 1: Add Rust AArch64 test requiring `net_rx=real_ok` with host UDP injection**
- [x] **Step 2: Add C AArch64 test requiring `net_rx=real_ok` with host UDP injection**
- [x] **Step 3: Confirm RED before implementation**

## Chunk 2: Runtime and Launcher Implementation

### Task 2: Add optional host UDP forwarding in AArch64 QEMU launcher

**Files:**
- Modify: `qemu/aarch64.sh`

- [x] **Step 1: Add `QOS_HOSTFWD_UDP_HOST_PORT` + `QOS_HOSTFWD_UDP_GUEST_PORT` env handling**
- [x] **Step 2: Append `hostfwd=udp::<host>-:<guest>` to `-netdev user,id=net0` when configured**

### Task 3: Implement Rust AArch64 RX probe path

**Files:**
- Modify: `tools/aarch64-probe/src/main.rs`

- [x] **Step 1: Add legacy RX queue memory/state and descriptor setup**
- [x] **Step 2: Configure legacy RX and TX queues in one device init sequence**
- [x] **Step 3: Poll RX used ring and classify `net_rx=real_ok|real_skip|real_fail`**
- [x] **Step 4: Preserve existing real TX proof (`net_tx=real_ok`)**

### Task 4: Implement C AArch64 RX probe path

**Files:**
- Modify: `tools/aarch64-c-probe/main.c`

- [x] **Step 1: Add legacy RX queue memory/state and descriptor setup**
- [x] **Step 2: Configure legacy RX and TX queues in one device init sequence**
- [x] **Step 3: Poll RX used ring and classify `net_rx=real_ok|real_skip|real_fail`**
- [x] **Step 4: Preserve existing real TX proof (`net_tx=real_ok`)**

## Chunk 3: Verification

### Task 5: Run targeted and full regression verification

**Files:**
- Modify: `docs/superpowers/plans/2026-03-12-phase82-aarch64-real-virtio-net-rx-proof.md`

- [x] **Step 1: Run `pytest -q tests/test_aarch64_real_net_rx.py`**
- [x] **Step 2: Run targeted AArch64 probe/smoke tests (`real_net_tx`, `real_net_rx`, runtime probe, smoke markers)**
- [x] **Step 3: Run full `make test-all`**
- [x] **Step 4: Run full `pytest -q`**
- [x] **Step 5: Run Rust verification (`cargo test -q -p qos-kernel`, `cargo test -q -p qos-libc`, `cargo check --workspace`)**

## Verification Results
- `pytest -q tests/test_aarch64_real_net_rx.py` -> pass (`2 passed`)
- `pytest -q tests/test_aarch64_real_net_tx.py tests/test_aarch64_real_net_rx.py tests/test_aarch64_runtime_probe.py tests/test_boot_handoff_smoke.py` -> pass (`7 passed`)
- `make test-all` -> pass (all 4 smoke targets)
- `pytest -q` -> pass (`95 passed`)
- `cd rust-os && cargo test -q -p qos-kernel` -> pass
- `cd rust-os && cargo test -q -p qos-libc` -> pass
- `cd rust-os && cargo check --workspace` -> pass
