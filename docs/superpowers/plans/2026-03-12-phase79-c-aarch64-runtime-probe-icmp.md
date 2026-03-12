# Phase 79 - C AArch64 Runtime Probe ICMP Alignment

**Goal:** Bring `c/aarch64` smoke behavior to runtime-probe parity with Rust by executing C kernel init + ICMP echo probe and surfacing `icmp_echo=gateway_ok` in real QEMU serial output.

**Architecture:** Replace the C aarch64 assembly-only marker path with a freestanding C probe ELF (`_start -> c_main`) that constructs `boot_info`, uses DTB pointer with deterministic fallback, calls `qos_kernel_entry`, then validates `qos_net_icmp_echo` and emits success/failure marker.

**Tech Stack:** AArch64 GCC/LD freestanding build, existing C kernel modules, QEMU smoke harness, pytest

## Tasks

### Task 1: Add RED contract for C runtime ICMP marker
- Modify: `tests/test_aarch64_runtime_probe.py`
- [x] **Step 1: Add `test_c_aarch64_smoke_reports_kernel_icmp_probe_marker`**
- [x] **Step 2: Run targeted test and confirm RED against old C stub output**

### Task 2: Implement C AArch64 probe build path
- Create: `tools/aarch64-c-probe/start.S`
- Create: `tools/aarch64-c-probe/main.c`
- Create: `tools/aarch64-c-probe/linker.ld`
- Modify: `scripts/build_aarch64_kernel.sh`
- [x] **Step 1: Add `_start` + stack setup in assembly and branch to `c_main(x0)`**
- [x] **Step 2: Add freestanding runtime helpers (`mem*`, `str*`) required by linked C kernel modules**
- [x] **Step 3: Add DTB fallback and `boot_info` population in `c_main`**
- [x] **Step 4: Call `qos_kernel_entry` and `qos_net_icmp_echo` to compute `icmp_echo` marker**
- [x] **Step 5: Wire `impl=c` branch in build script to compile/link probe ELF instead of asm stub**

### Task 3: Verify matrix stability
- [x] **Step 1: Run `make -C c-os ARCH=aarch64 smoke` and confirm marker output**
- [x] **Step 2: Run `pytest -q tests/test_aarch64_runtime_probe.py`**
- [x] **Step 3: Run full verification (`make test-all`, `pytest -q`, rust kernel/libc tests, workspace check)**

## Verification Results
- `make -C c-os ARCH=aarch64 smoke` -> pass (log includes `icmp_echo=gateway_ok`)
- `pytest -q tests/test_aarch64_runtime_probe.py` -> pass (`2 passed`)
- `make test-all` -> pass (all 4 smoke targets)
- `pytest -q` -> pass (`91 passed`)
- `cd rust-os && cargo test -q -p qos-kernel` -> pass
- `cd rust-os && cargo test -q -p qos-libc` -> pass
- `cd rust-os && cargo check --workspace` -> pass
