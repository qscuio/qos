# Phase 78 - Rust AArch64 Runtime Probe in QEMU

**Goal:** Replace the Rust AArch64 smoke stub path with a real Rust runtime probe that executes kernel init + ICMP echo and emits the required serial marker (`icmp_echo=gateway_ok`).

**Architecture:** Boot a dedicated `no_std` AArch64 probe ELF (`_start -> rust_main`), build `BootInfo` in Rust, call `qos_kernel::kernel_entry`, run `net_icmp_echo`, and print deterministic smoke markers from UART. Use a custom linker script and soft-float target to avoid early EL FP trap issues before EL bring-up.

**Tech Stack:** Rust `no_std`, global AArch64 asm, QEMU smoke harness, pytest

## Tasks

### Task 1: Add RED assertion for runtime marker
- Create: `tests/test_aarch64_runtime_probe.py`
- [x] **Step 1: Require `icmp_echo=gateway_ok` in `rust-os/build/aarch64/smoke.log`**
- [x] **Step 2: Run targeted pytest and observe RED against old stub path**

### Task 2: Implement runtime probe and wire build path
- Create: `tools/aarch64-probe/Cargo.toml`
- Create: `tools/aarch64-probe/src/main.rs`
- Create: `tools/aarch64-probe/linker.ld`
- Modify: `scripts/build_aarch64_kernel.sh`
- [x] **Step 1: Add `_start` + stack setup and call into `rust_main`**
- [x] **Step 2: Build `BootInfo`, execute `kernel_entry`, execute `net_icmp_echo`**
- [x] **Step 3: Emit success/failure boot marker including `icmp_echo=*`**
- [x] **Step 4: Use deterministic DTB fallback blob and pass effective DTB via boot info**
- [x] **Step 5: Build Rust probe as `--release` on `aarch64-unknown-none-softfloat` with linker script**

### Task 3: Verify end-to-end matrix stability
- [x] **Step 1: Run `make -C rust-os ARCH=aarch64 smoke` and confirm serial success marker**
- [x] **Step 2: Run `pytest -q tests/test_aarch64_runtime_probe.py::test_rust_aarch64_smoke_reports_kernel_icmp_probe_marker`**
- [x] **Step 3: Run full verification (`make test-all`, `pytest -q`, rust kernel/libc tests, workspace check)**

## Verification Results
- `make test-all` -> pass (all 4 smoke targets)
- `pytest -q` -> pass (`90 passed`)
- `pytest -q tests/test_aarch64_runtime_probe.py::test_rust_aarch64_smoke_reports_kernel_icmp_probe_marker` -> pass
- `cd rust-os && cargo test -q -p qos-kernel` -> pass
- `cd rust-os && cargo test -q -p qos-libc` -> pass
- `cd rust-os && cargo check --workspace` -> pass
