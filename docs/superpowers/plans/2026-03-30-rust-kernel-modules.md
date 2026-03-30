# Rust Kernel Modules Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the six advanced Rust kernel module samples described in the approved 2026-03-30 design, wire them into linux-lab build/config/catalog assets, and provide companion helper coverage that is verifiable from this repo.

**Architecture:** Keep the existing `linux-lab/examples/rust/rust_learn/` layout and extend it with six new kernel sample sources plus companion helper scripts. Treat the repo-verifiable contract as the red/green loop in this environment: catalog visibility, planner exposure, config integration, helper buildability, and presence of the requested Rust sample sources and scripts.

**Tech Stack:** Linux-lab orchestrator Python, Kconfig/Makefile fragments, Rust kernel sample sources, Rust userspace helpers, shell scripts, pytest

---

## Chunk 1: Repo-Level Contracts

### Task 1: Add failing tests for the expanded Rust sample surface

**Files:**
- Modify: `tests/test_linux_lab_example_assets.py`
- Modify: `tests/test_linux_lab_example_catalog.py`

- [ ] **Step 1: Write the failing tests**
- [ ] **Step 2: Run the targeted pytest cases and confirm red**
  Run: `python -m pytest tests/test_linux_lab_example_assets.py tests/test_linux_lab_example_catalog.py -q`
- [ ] **Step 3: Extend the assertions to cover**
  - new `.rs` sample files
  - new helper scripts directory contents
  - new `CONFIG_SAMPLE_RUST_*` and planner sample-source exposure
  - catalog entry stability for `rust_learn`
- [ ] **Step 4: Re-run the targeted pytest cases after implementation and confirm green**

## Chunk 2: Build Wiring And Helper Assets

### Task 2: Wire new samples into linux-lab integration files

**Files:**
- Modify: `linux-lab/examples/rust/rust_learn/Kconfig`
- Modify: `linux-lab/examples/rust/rust_learn/Makefile`
- Modify: `linux-lab/fragments/rust.conf`
- Modify: `linux-lab/catalog/examples/rust_learn.yaml`
- Modify: `linux-lab/examples/rust/rust_learn/README.md`

- [ ] **Step 1: Add all six Kconfig entries**
- [ ] **Step 2: Add all six Makefile object rules**
- [ ] **Step 3: Add config-fragment module enables**
- [ ] **Step 4: Update catalog tags/notes to reflect the expanded sample set**
- [ ] **Step 5: Refresh the README module inventory and quick-test guidance**

### Task 3: Add companion helper scripts and userspace helper wiring

**Files:**
- Create: `linux-lab/examples/rust/rust_learn/scripts/*.sh`
- Modify: `linux-lab/examples/rust/rust_learn/user/Makefile`
- Create/Modify: `linux-lab/examples/rust/rust_learn/user/*.rs`

- [ ] **Step 1: Add helper scripts for the procfs/platform/slab/netdev paths and any misc-device smoke paths that benefit from userspace helpers**
- [ ] **Step 2: Add Rust userspace helper binaries for the ioctl-driven modules where a small binary is clearer than shell**
- [ ] **Step 3: Build the userspace helper directory and confirm it still compiles**
  Run: `make -C linux-lab/examples/rust/rust_learn/user`

## Chunk 3: Kernel Sample Sources

### Task 4: Implement `rust_sync` and `rust_workqueue`

**Files:**
- Create: `linux-lab/examples/rust/rust_learn/rust_sync.rs`
- Create: `linux-lab/examples/rust/rust_learn/rust_workqueue.rs`

- [ ] **Step 1: Add full module doc-comments with ioctl usage**
- [ ] **Step 2: Implement the misc-device state, shared counters, and lifecycle logging described by the spec**
- [ ] **Step 3: Keep unsafe usage minimal and document each safety boundary**

### Task 5: Implement `rust_procfs` and `rust_platform`

**Files:**
- Create: `linux-lab/examples/rust/rust_learn/rust_procfs.rs`
- Create: `linux-lab/examples/rust/rust_learn/rust_platform.rs`

- [ ] **Step 1: Add procfs wrappers, stats/config behavior, and cleanup hooks**
- [ ] **Step 2: Add platform-driver sample structure, simulated register access, and misc-device exposure**
- [ ] **Step 3: Document kernel-version assumptions and safety notes inline**

### Task 6: Implement `rust_slab` and `rust_netdev`

**Files:**
- Create: `linux-lab/examples/rust/rust_learn/rust_slab.rs`
- Create: `linux-lab/examples/rust/rust_learn/rust_netdev.rs`

- [ ] **Step 1: Add handle-based slab cache sample with generation-checked slots**
- [ ] **Step 2: Add virtual netdev sample skeleton with TX/RX/accounting/carrier controls**
- [ ] **Step 3: Document every FFI-heavy boundary clearly because these are the highest-risk samples in the set**

## Chunk 4: Verification

### Task 7: Run final verification available in this repo

**Files:**
- Test: `tests/test_linux_lab_example_assets.py`
- Test: `tests/test_linux_lab_example_catalog.py`
- Test: `linux-lab/examples/rust/rust_learn/user/Makefile`

- [ ] **Step 1: Run targeted pytest for the example asset/catalog surface**
  Run: `python -m pytest tests/test_linux_lab_example_assets.py tests/test_linux_lab_example_catalog.py -q`
- [ ] **Step 2: Run the rust_learn userspace helper build**
  Run: `make -C linux-lab/examples/rust/rust_learn/user clean all`
- [ ] **Step 3: If a local kernel workspace is unavailable, explicitly record that kernel-module compilation was not executable in this environment**
