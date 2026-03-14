# Linux Lab Phase 2 Kernel Assets Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port real kernel patch/config assets from `../qulk` into `linux-lab` and add tested helper logic for source fetch verification, patch application, and config composition.

**Architecture:** Phase 2 keeps the existing Phase 1 CLI contract intact. Real kernel assets move into `linux-lab/patches/`, `linux-lab/configs/`, and `linux-lab/fragments/`, while new Python helpers under `linux-lab/orchestrator/core/` implement deterministic local fetch, extract, patch, and config-merge behavior. Stage modules may call these helpers later, but Phase 2 verification focuses on the helpers and asset wiring rather than network-boot execution.

**Tech Stack:** Python 3.12, PyYAML, pytest, `tar`, `sha256sum`, `patch`

---

## Chunk 1: Kernel Assets

### Task 1: Add failing Phase 2 asset and helper tests

**Files:**
- Create: `tests/test_linux_lab_phase2_assets.py`
- Create: `tests/test_linux_lab_phase2_kernel_assets.py`
- Reference: `docs/superpowers/specs/2026-03-14-linux-lab-design.md`

- [ ] **Step 1: Write failing asset tests for patch/config migration**
- [ ] **Step 2: Write failing helper tests for fetch verification, patch apply, and config merge**
- [ ] **Step 3: Run tests to verify they fail**

Run:
```bash
pytest tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py -v
```

Expected: FAIL on missing files and missing helper module behavior.

- [ ] **Step 4: Commit**

```bash
git add tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py
git commit -m "test: add linux-lab phase2 kernel asset coverage"
```

### Task 2: Port patch files, baseline configs, and Phase 2 fragments

**Files:**
- Create: `linux-lab/patches/linux-4.19.317/kernel.patch`
- Create: `linux-lab/patches/linux-6.4.3/kernel.patch`
- Create: `linux-lab/patches/linux-6.9.6/kernel.patch`
- Create: `linux-lab/patches/linux-6.9.8/kernel.patch`
- Create: `linux-lab/patches/linux-6.10/kernel.patch`
- Create: `linux-lab/configs/x86_64/defconfig`
- Create: `linux-lab/configs/arm64/defconfig`
- Create: `linux-lab/fragments/debug.conf`
- Create: `linux-lab/fragments/bpf.conf`
- Create: `linux-lab/fragments/rust.conf`
- Create: `linux-lab/fragments/samples.conf`
- Create: `linux-lab/fragments/debug-tools.conf`
- Create: `linux-lab/fragments/minimal.conf`
- Modify: `linux-lab/manifests/kernels/*.yaml`
- Modify: `linux-lab/manifests/profiles/*.yaml`

- [ ] **Step 1: Run asset tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_phase2_assets.py -v
```

Expected: FAIL on missing patch/config/fragment files and manifest refs.

- [ ] **Step 2: Port exact patch/config assets and wire manifest refs**
- [ ] **Step 3: Translate `../qulk/ulk` config mutations into fragment files**
- [ ] **Step 4: Re-run asset tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_phase2_assets.py -v
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add linux-lab/patches linux-lab/configs linux-lab/fragments linux-lab/manifests tests/test_linux_lab_phase2_assets.py
git commit -m "feat: port linux-lab kernel patches and config assets"
```

### Task 3: Implement tested kernel asset helpers

**Files:**
- Create: `linux-lab/orchestrator/core/kernel_assets.py`
- Test: `tests/test_linux_lab_phase2_kernel_assets.py`

- [ ] **Step 1: Run helper tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_phase2_kernel_assets.py -v
```

Expected: FAIL on missing helper module and behaviors.

- [ ] **Step 2: Implement minimal helpers**

Required helpers:
- `fetch_kernel_archive(...)`
- `extract_kernel_archive(...)`
- `apply_kernel_patch(...)`
- `merge_kernel_config(...)`
- `resolve_profile_fragment_paths(...)`

- [ ] **Step 3: Re-run helper tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_phase2_kernel_assets.py -v
```

Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add linux-lab/orchestrator/core/kernel_assets.py tests/test_linux_lab_phase2_kernel_assets.py
git commit -m "feat: add linux-lab kernel asset helpers"
```

### Task 4: Integrate Phase 2 helper awareness without changing the Phase 1 CLI contract

**Files:**
- Modify: `linux-lab/orchestrator/core/manifests.py`
- Modify: `linux-lab/orchestrator/core/stages.py`
- Modify: `README.md`
- Test: `tests/test_linux_lab_phase1_repo.py`
- Test: `tests/test_linux_lab_phase2_assets.py`

- [ ] **Step 1: Add failing assertions for helper-aware manifest/stage metadata**
- [ ] **Step 2: Wire canonical patch/config/fragment paths into the stage planning metadata**
- [ ] **Step 3: Re-run relevant tests**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py -v
```

Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add linux-lab/orchestrator/core/manifests.py linux-lab/orchestrator/core/stages.py README.md tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase2_assets.py
git commit -m "feat: wire linux-lab phase2 kernel asset metadata"
```

### Final Verification

- [ ] **Step 1: Run Phase 2 targeted verification**

Run:
```bash
pytest tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py -v
```

Expected: PASS

- [ ] **Step 2: Run Phase 1 regression coverage**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py -v
```

Expected: PASS

- [ ] **Step 3: Note deferred work**

Deferred to later phases:
- real kernel build execution
- image build execution
- QEMU boot execution
- example project onboarding
