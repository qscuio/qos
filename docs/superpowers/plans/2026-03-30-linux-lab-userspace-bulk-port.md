# Linux Lab Userspace Bulk Port Implementation Plan

> **For agentic workers:** Execute in sequence. This session cannot use subagent review without explicit user authorization, so the author must self-review each chunk before moving on.

**Goal:** Import the full `qulk/userspace/app` corpus into `linux-lab`, port the dependent shared lib and vendored dependency closure, expose a single bulk build target, and fix compile failures until the integrated build succeeds.

**Architecture:** Keep the imported source recognizable under `linux-lab/examples/userspace/qulk/`, add one top-level bulk build entrypoint there, and wire the orchestrator to invoke it as a `custom-make` userspace example instead of compiling standalone `.c` files one by one.

**Tech Stack:** GNU make, gcc/clang toolchain, Python 3, pytest, bash

---

## Chunk 1: Lock The Contract

### Task 1: Add failing tests for the new userspace bulk entry

**Files:**
- Modify: `tests/test_linux_lab_example_catalog.py`
- Modify: `tests/test_linux_lab_example_assets.py`
- Reference: `docs/superpowers/specs/2026-03-30-linux-lab-userspace-bulk-port-design.md`

- [ ] **Step 1: Add a failing catalog assertion for the bulk userspace entry**

```python
def test_userspace_bulk_entry_exists() -> None:
    catalog = load_example_catalog(ROOT / "linux-lab" / "catalog" / "examples")
    item = catalog["userspace-qulk-bulk"]
    assert item["kind"] == "userspace"
    assert item["build_mode"] == "custom-make"
```

- [ ] **Step 2: Add a failing asset test for the imported subtree**

```python
def test_userspace_qulk_tree_exists() -> None:
    root = ROOT / "linux-lab" / "examples" / "userspace" / "qulk"
    assert (root / "app").is_dir()
    assert (root / "lib").is_dir()
    assert (root / "Makefile").is_file()
```

- [ ] **Step 3: Run the focused tests and verify failure**

Run:
```bash
pytest tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py -k userspace -v
```

Expected: FAIL on missing bulk entry and missing imported subtree.

## Chunk 2: Import The Source Closure

### Task 2: Import the app corpus, shared lib layer, and vendored dependencies

**Files:**
- Create: `linux-lab/examples/userspace/qulk/**`
- Create: `linux-lab/examples/userspace/qulk/README.md`
- Test: `tests/test_linux_lab_example_assets.py`

- [ ] **Step 1: Copy the full top-level app corpus into `linux-lab/examples/userspace/qulk/app/`**
- [ ] **Step 2: Copy the shared `qulk/userspace/lib` layer into `linux-lab/examples/userspace/qulk/lib/`**
- [ ] **Step 3: Copy the vendored dependency trees needed by the app/lib closure into `linux-lab/examples/userspace/qulk/vendor/`**
- [ ] **Step 4: Write a short README describing source origin and dependency policy**
- [ ] **Step 5: Re-run the asset tests and confirm the imported tree is present**

Run:
```bash
pytest tests/test_linux_lab_example_assets.py -k userspace -v
```

Expected: PASS for tree-presence assertions, while catalog/build assertions still fail.

## Chunk 3: Build One Bulk Target

### Task 3: Add the bulk build system and catalog entry

**Files:**
- Create: `linux-lab/examples/userspace/qulk/Makefile`
- Create: `linux-lab/examples/userspace/qulk/tools/build-manifest.py`
- Create: `linux-lab/catalog/examples/userspace-qulk-bulk.yaml`
- Modify: `linux-lab/catalog/examples/userspace-app.yaml`
- Modify: `tests/test_linux_lab_example_catalog.py`

- [ ] **Step 1: Create the top-level userspace bulk `Makefile` with layered build directories**
- [ ] **Step 2: Teach the build to emit `build/manifest.json` summarizing outputs**
- [ ] **Step 3: Add the `userspace-qulk-bulk` catalog entry with `build_mode: custom-make`**
- [ ] **Step 4: Decide whether to keep or retire the old curated `userspace-app` entry and update tests accordingly**
- [ ] **Step 5: Run the focused catalog tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_example_catalog.py -k userspace -v
```

Expected: PASS for the bulk-entry contract.

## Chunk 4: Wire The Orchestrator

### Task 4: Integrate the bulk userspace target with example planning

**Files:**
- Modify: `linux-lab/orchestrator/core/examples.py`
- Modify: `tests/test_linux_lab_live_tools_examples.py`
- Modify: `tests/test_linux_lab_example_catalog.py`

- [ ] **Step 1: Update userspace planning so `custom-make` userspace entries run their configured build command instead of standalone `gcc` compilation**
- [ ] **Step 2: Preserve existing `gcc-userspace` behavior for small curated entries if they remain**
- [ ] **Step 3: Add or adjust tests that validate the planned build command for the bulk userspace entry**
- [ ] **Step 4: Run the planner tests and verify pass**

Run:
```bash
pytest tests/test_linux_lab_live_tools_examples.py tests/test_linux_lab_example_catalog.py -k userspace -v
```

Expected: PASS

## Chunk 5: Make The Imported Corpus Compile

### Task 5: Add the shared compatibility/build layer

**Files:**
- Create: `linux-lab/examples/userspace/qulk/build/include/**`
- Create: `linux-lab/examples/userspace/qulk/build-rules.mk`
- Modify: `linux-lab/examples/userspace/qulk/Makefile`

- [ ] **Step 1: Add centralized include paths, compiler flags, and linker groups**
- [ ] **Step 2: Add generated or shim headers required by imported shared-lib code**
- [ ] **Step 3: Build the internal support library objects first**
- [ ] **Step 4: Build the vendored dependency libraries next**
- [ ] **Step 5: Attempt the first full bulk build and capture the failure set**

Run:
```bash
make -C linux-lab/examples/userspace/qulk -j"$(nproc)"
```

Expected: FAIL with the first real compile/link errors from the imported corpus.

### Task 6: Iterate on compile fixes until the bulk build succeeds

**Files:**
- Modify: `linux-lab/examples/userspace/qulk/app/**`
- Modify: `linux-lab/examples/userspace/qulk/lib/**`
- Modify: `linux-lab/examples/userspace/qulk/vendor/**`
- Modify: `linux-lab/examples/userspace/qulk/Makefile`
- Modify: `linux-lab/examples/userspace/qulk/build-rules.mk`

- [ ] **Step 1: Fix include-path and generated-header failures**
- [ ] **Step 2: Fix vendored dependency integration issues**
- [ ] **Step 3: Fix stale prototypes, duplicate symbols, and Linux toolchain compatibility problems in source**
- [ ] **Step 4: Re-run the bulk build after each failure batch**
- [ ] **Step 5: Record any explicitly unsupported files in the build manifest or README**
- [ ] **Step 6: Continue until the bulk target completes successfully**

Run:
```bash
make -C linux-lab/examples/userspace/qulk clean
make -C linux-lab/examples/userspace/qulk -j"$(nproc)"
```

Expected: PASS, with `build/manifest.json` and the built binaries present under `build/bin/`.

## Chunk 6: Final Verification

### Task 7: Verify repo integration and bulk userspace build coverage

**Files:**
- Modify: `tests/test_linux_lab_example_assets.py`
- Modify: `tests/test_linux_lab_live_tools_examples.py`
- Modify: `linux-lab/examples/userspace/qulk/README.md`

- [ ] **Step 1: Add a test for the manifest output path or build command contract**
- [ ] **Step 2: Run the targeted linux-lab tests**

Run:
```bash
pytest tests/test_linux_lab_example_assets.py tests/test_linux_lab_example_catalog.py tests/test_linux_lab_live_tools_examples.py -v
```

Expected: PASS

- [ ] **Step 3: Run the full userspace bulk build one more time from a clean tree**

Run:
```bash
make -C linux-lab/examples/userspace/qulk clean
make -C linux-lab/examples/userspace/qulk -j"$(nproc)"
```

Expected: PASS

- [ ] **Step 4: Review `git diff --stat` and confirm unrelated files were not modified**

