# Linux Lab Runtime Parity Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish the remaining `linux-lab` port by adding runtime-capable image/QEMU orchestration, curated example parity for modules/userspace/Rust/BPF, and on-demand debug tool workflows.

**Architecture:** Keep `plan` as a metadata-only command and add a native `run` command that uses the same manifest resolution but can either execute stages or emit a dry-run script plan. Runtime behavior stays manifest-driven: images, tools, examples, and boot parameters are declared in repo assets and resolved into deterministic stage outputs under `build/linux-lab/requests/<fingerprint>/`.

**Tech Stack:** Python 3.12, PyYAML, pytest, GNU make, `patch`, `tar`, `debootstrap`, `qemu-system-x86_64`, `qemu-system-aarch64`, `git`

---

## Chunk 1: Runtime CLI And Image/QEMU Assets

### Task 1: Add failing runtime/image parity tests

**Files:**
- Create: `tests/test_linux_lab_runtime_cli.py`
- Create: `tests/test_linux_lab_image_assets.py`
- Reference: `docs/superpowers/specs/2026-03-14-linux-lab-design.md`

- [ ] **Step 1: Write failing tests for the native `run` command contract**
- [ ] **Step 2: Write failing tests for image asset parity and boot command generation**
- [ ] **Step 3: Run tests to verify they fail**

Run:
```bash
pytest tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py -v
```

Expected: FAIL on missing `run` command support, missing image assets, and missing boot/image planning metadata.

- [ ] **Step 4: Commit**

```bash
git add tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py
git commit -m "test: add linux-lab runtime parity coverage"
```

### Task 2: Port image assets and implement image/QEMU planning helpers

**Files:**
- Create: `linux-lab/images/releases/jammy/01-netcfg.yaml`
- Create: `linux-lab/images/releases/jammy/sources.list`
- Create: `linux-lab/images/releases/noble/01-netcfg.yaml`
- Create: `linux-lab/images/releases/noble/sources.list`
- Create: `linux-lab/orchestrator/core/image_assets.py`
- Create: `linux-lab/orchestrator/core/qemu.py`
- Modify: `linux-lab/manifests/images/jammy.yaml`
- Modify: `linux-lab/manifests/images/noble.yaml`
- Modify: `linux-lab/manifests/arches/x86_64.yaml`
- Modify: `linux-lab/manifests/arches/arm64.yaml`

- [ ] **Step 1: Run the image/runtime tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py -v
```

Expected: FAIL on missing assets and helper modules.

- [ ] **Step 2: Port the jammy/noble networking and sources assets from `../qulk/configs/`**
- [ ] **Step 3: Implement mirror resolution, image path planning, and QEMU command generation helpers**
- [ ] **Step 4: Re-run the image/runtime tests and confirm the asset/helper slice passes**

Run:
```bash
pytest tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py -v
```

Expected: PASS for asset and helper coverage, with runtime-command tests still failing on missing CLI/stage wiring if not yet implemented.

- [ ] **Step 5: Commit**

```bash
git add linux-lab/images linux-lab/manifests/arches linux-lab/manifests/images linux-lab/orchestrator/core/image_assets.py linux-lab/orchestrator/core/qemu.py tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py
git commit -m "feat: add linux-lab image and qemu assets"
```

### Task 3: Implement native `run` and stage-aware dry-run/runtime metadata

**Files:**
- Modify: `linux-lab/orchestrator/cli.py`
- Modify: `linux-lab/orchestrator/core/request.py`
- Modify: `linux-lab/orchestrator/core/state.py`
- Modify: `linux-lab/orchestrator/core/stages.py`
- Modify: `linux-lab/orchestrator/stages/build_kernel.py`
- Modify: `linux-lab/orchestrator/stages/build_image.py`
- Modify: `linux-lab/orchestrator/stages/boot.py`
- Test: `tests/test_linux_lab_runtime_cli.py`
- Test: `tests/test_linux_lab_image_assets.py`

- [ ] **Step 1: Run the runtime CLI tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_runtime_cli.py -v
```

Expected: FAIL on missing `run`, `--dry-run`, and `--stop-after`.

- [ ] **Step 2: Extend the request model for `run` execution mode with dry-run and stop-after controls**
- [ ] **Step 3: Implement `run` so it emits the same stage state as `plan`, plus executable command metadata for build-kernel/build-image/boot**
- [ ] **Step 4: Keep `validate` read-only and `plan` metadata-only**
- [ ] **Step 5: Re-run runtime/image tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py -v
```

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add linux-lab/orchestrator/cli.py linux-lab/orchestrator/core/request.py linux-lab/orchestrator/core/state.py linux-lab/orchestrator/core/stages.py linux-lab/orchestrator/stages/build_kernel.py linux-lab/orchestrator/stages/build_image.py linux-lab/orchestrator/stages/boot.py tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py
git commit -m "feat: add linux-lab runtime command flow"
```

## Chunk 2: Tool And Example Parity

### Task 4: Add failing tool/example parity tests

**Files:**
- Create: `tests/test_linux_lab_tool_assets.py`
- Create: `tests/test_linux_lab_example_assets.py`

- [ ] **Step 1: Write failing tests for tool manifests and example assets**
- [ ] **Step 2: Write failing tests for build-tools and build-examples stage metadata**
- [ ] **Step 3: Run the tests to verify they fail**

Run:
```bash
pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py -v
```

Expected: FAIL on missing tool manifests, missing example assets, and placeholder stage metadata.

- [ ] **Step 4: Commit**

```bash
git add tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py
git commit -m "test: add linux-lab tool and example parity coverage"
```

### Task 5: Port curated tools and examples

**Files:**
- Create: `linux-lab/tools/crash.yaml`
- Create: `linux-lab/tools/cgdb.yaml`
- Create: `linux-lab/tools/libbpf-bootstrap.yaml`
- Create: `linux-lab/tools/retsnoop.yaml`
- Create: `linux-lab/examples/modules/debug/Makefile`
- Create: `linux-lab/examples/modules/debug/*.c`
- Create: `linux-lab/examples/modules/simple/Makefile`
- Create: `linux-lab/examples/modules/simple/main.c`
- Create: `linux-lab/examples/modules/ioctl/Makefile`
- Create: `linux-lab/examples/modules/ioctl/*.c`
- Create: `linux-lab/examples/userspace/app/*.c`
- Create: `linux-lab/examples/bpf/learn/*.c`
- Create: `linux-lab/examples/rust/rust_learn/*`
- Create: `linux-lab/orchestrator/core/tooling.py`
- Create: `linux-lab/orchestrator/core/examples.py`
- Modify: `README.md`

- [ ] **Step 1: Port a curated, build-focused example set from `../qulk`**
- [ ] **Step 2: Add tool manifests for the external repos `ulk` provisioned**
- [ ] **Step 3: Implement helper functions that resolve clone/build commands for tools and example groups**
- [ ] **Step 4: Re-run tool/example parity tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py -v
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add linux-lab/tools linux-lab/examples linux-lab/orchestrator/core/tooling.py linux-lab/orchestrator/core/examples.py README.md tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py
git commit -m "feat: port linux-lab tools and curated examples"
```

### Task 6: Wire build-tools and build-examples stages to real runtime metadata

**Files:**
- Modify: `linux-lab/orchestrator/stages/build_tools.py`
- Modify: `linux-lab/orchestrator/stages/build_examples.py`
- Modify: `linux-lab/orchestrator/stages/configure.py`
- Test: `tests/test_linux_lab_tool_assets.py`
- Test: `tests/test_linux_lab_example_assets.py`
- Test: `tests/test_linux_lab_runtime_cli.py`

- [ ] **Step 1: Run the new stage tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_runtime_cli.py -v
```

Expected: FAIL on placeholder tool/example stage metadata.

- [ ] **Step 2: Emit deterministic tool/example plans, clone/build commands, and Rust sample sync metadata**
- [ ] **Step 3: Re-run the stage tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_runtime_cli.py -v
```

Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add linux-lab/orchestrator/stages/build_tools.py linux-lab/orchestrator/stages/build_examples.py linux-lab/orchestrator/stages/configure.py tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_runtime_cli.py
git commit -m "feat: wire linux-lab tool and example stages"
```

## Chunk 3: Final Integration

### Task 7: Add top-level integration checks and full verification

**Files:**
- Modify: `README.md`
- Modify: `Makefile`
- Modify: `.gitignore`
- Modify: `tests/test_linux_lab_phase1_repo.py`
- Create: `tests/test_linux_lab_full_plan.py`

- [ ] **Step 1: Add failing coverage for the finished runtime-oriented repo contract**
- [ ] **Step 2: Update docs and make targets for `linux-lab run`**
- [ ] **Step 3: Add a full dry-run integration test that exercises the complete stage graph**
- [ ] **Step 4: Re-run the integration coverage**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_full_plan.py -v
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add README.md Makefile .gitignore tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_full_plan.py
git commit -m "feat: finish linux-lab runtime integration"
```

### Final Verification

- [ ] **Step 1: Run the complete linux-lab test slice**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_full_plan.py -v
```

Expected: PASS

- [ ] **Step 2: Run representative CLI verification**

Run:
```bash
linux-lab/bin/linux-lab validate --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab
linux-lab/bin/linux-lab plan --kernel 6.9.8 --arch arm64 --image jammy --profile debug --profile rust --mirror sg
linux-lab/bin/linux-lab run --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab --dry-run
linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg
```

Expected: All commands exit `0`

- [ ] **Step 3: Note any intentionally deferred heavy-runtime execution**

Deferred only if the environment lacks host dependencies:
- full kernel compile on every supported arch
- debootstrap image creation requiring elevated host tools
- live QEMU boot of the produced image
