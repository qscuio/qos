# Linux Lab Live Execution Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert `linux-lab run` from dry-run metadata into real staged execution, including kernel fetch/patch/configure/build and the image/boot helper assets needed for live workflows.

**Architecture:** Keep `validate` read-only and `plan` metadata-only. Extend `run` so non-dry-run requests execute stage logic in order, with each stage writing status, logs, and artifact metadata under the existing request root. Use Python helpers for archive/config/build orchestration and port the small shell helper surface from `qulk` only where it remains the practical runtime boundary for image creation, QEMU boot, and guest connect/debug helpers.

**Tech Stack:** Python 3.12, PyYAML, pytest, GNU make, `patch`, `tar`, `git`, `debootstrap`, `qemu-system-x86_64`, `qemu-system-aarch64`

---

## Chunk 1: Live Kernel Stages

### Task 1: Add failing tests for non-dry-run kernel stage execution

**Files:**
- Create: `tests/test_linux_lab_live_execution.py`
- Reference: `linux-lab/orchestrator/stages/fetch.py`
- Reference: `linux-lab/orchestrator/stages/patch.py`
- Reference: `linux-lab/orchestrator/stages/configure.py`
- Reference: `linux-lab/orchestrator/stages/build_kernel.py`

- [ ] **Step 1: Write failing tests for real fetch, patch, configure, and build-kernel stage execution**
- [ ] **Step 2: Use temporary fake kernel archives/trees so the tests run quickly and deterministically**
- [ ] **Step 3: Run the new tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_live_execution.py -v
```

Expected: FAIL on missing non-dry-run execution behavior.

- [ ] **Step 4: Commit**

```bash
git add tests/test_linux_lab_live_execution.py
git commit -m "test: add linux-lab live execution coverage"
```

### Task 2: Implement real fetch/patch/configure/build execution

**Files:**
- Modify: `linux-lab/orchestrator/core/stages.py`
- Modify: `linux-lab/orchestrator/stages/fetch.py`
- Modify: `linux-lab/orchestrator/stages/patch.py`
- Modify: `linux-lab/orchestrator/stages/configure.py`
- Modify: `linux-lab/orchestrator/stages/build_kernel.py`
- Modify: `linux-lab/orchestrator/core/kernel_assets.py`
- Test: `tests/test_linux_lab_live_execution.py`

- [ ] **Step 1: Run the live-execution tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_live_execution.py -v
```

Expected: FAIL

- [ ] **Step 2: Add generic command execution helpers with stage log capture**
- [ ] **Step 3: Make `fetch` download and verify archives in non-dry-run mode**
- [ ] **Step 4: Make `patch` extract trees and apply kernel patches in non-dry-run mode**
- [ ] **Step 5: Make `configure` write merged `.config` outputs in non-dry-run mode**
- [ ] **Step 6: Make `build-kernel` execute its generated `make` commands in non-dry-run mode**
- [ ] **Step 7: Re-run the live-execution tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_live_execution.py -v
```

Expected: PASS

- [ ] **Step 8: Commit**

```bash
git add linux-lab/orchestrator/core/stages.py linux-lab/orchestrator/stages/fetch.py linux-lab/orchestrator/stages/patch.py linux-lab/orchestrator/stages/configure.py linux-lab/orchestrator/stages/build_kernel.py linux-lab/orchestrator/core/kernel_assets.py tests/test_linux_lab_live_execution.py
git commit -m "feat: add linux-lab live kernel stage execution"
```

## Chunk 2: Image And Boot Helper Assets

### Task 3: Add failing tests for image/boot helper scripts and stage runtime metadata

**Files:**
- Create: `tests/test_linux_lab_boot_assets.py`
- Reference: `linux-lab/orchestrator/stages/build_image.py`
- Reference: `linux-lab/orchestrator/stages/boot.py`

- [ ] **Step 1: Write failing tests for ported image/boot helper scripts**
- [ ] **Step 2: Write failing tests for non-dry-run build-image/boot command generation**
- [ ] **Step 3: Run the tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_boot_assets.py -v
```

Expected: FAIL on missing scripts and incomplete runtime metadata.

- [ ] **Step 4: Commit**

```bash
git add tests/test_linux_lab_boot_assets.py
git commit -m "test: add linux-lab boot asset coverage"
```

### Task 4: Port helper scripts and integrate runtime stage commands

**Files:**
- Create: `linux-lab/scripts/create-image.sh`
- Create: `linux-lab/scripts/boot.sh`
- Create: `linux-lab/scripts/connect`
- Create: `linux-lab/scripts/gdb`
- Create: `linux-lab/scripts/up.sh`
- Create: `linux-lab/scripts/down.sh`
- Modify: `linux-lab/orchestrator/core/image_assets.py`
- Modify: `linux-lab/orchestrator/core/qemu.py`
- Modify: `linux-lab/orchestrator/stages/build_image.py`
- Modify: `linux-lab/orchestrator/stages/boot.py`
- Modify: `README.md`
- Test: `tests/test_linux_lab_boot_assets.py`

- [ ] **Step 1: Port the runtime helper scripts from `../qulk/scripts/` into `linux-lab/scripts/`**
- [ ] **Step 2: Adjust them for the new `linux-lab` artifact layout and generated request roots**
- [ ] **Step 3: Make build-image and boot runtime stages reference and optionally execute those scripts**
- [ ] **Step 4: Re-run the boot asset tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_boot_assets.py -v
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add linux-lab/scripts linux-lab/orchestrator/core/image_assets.py linux-lab/orchestrator/core/qemu.py linux-lab/orchestrator/stages/build_image.py linux-lab/orchestrator/stages/boot.py README.md tests/test_linux_lab_boot_assets.py
git commit -m "feat: port linux-lab image and boot scripts"
```

## Chunk 3: Final Runtime Verification

### Task 5: Run live stage verification and regressions

**Files:**
- Modify only as required by verification findings

- [ ] **Step 1: Run the full linux-lab pytest slice**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_full_plan.py tests/test_linux_lab_live_execution.py tests/test_linux_lab_boot_assets.py -v
```

Expected: PASS

- [ ] **Step 2: Run representative live commands**

Run:
```bash
linux-lab/bin/linux-lab run --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab --dry-run
linux-lab/bin/linux-lab run --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab --stop-after configure
linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg
make linux-lab-run
```

Expected: exit `0`

- [ ] **Step 3: If host dependencies allow, note which non-dry-run stages were executed successfully**
