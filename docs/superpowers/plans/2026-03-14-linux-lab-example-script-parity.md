# Linux Lab Example And Script Parity Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Import the full `qulk` sample corpus and the missing guest/runtime helper scripts into `linux-lab`, then switch example planning from a curated hardcoded subset to a catalog-driven model.

**Architecture:** Keep imported sample trees mostly intact inside the new `linux-lab/examples/...` hierarchy, and describe them through per-sample catalog YAML files under `linux-lab/catalog/examples/`. Keep common operator command names at `linux-lab/scripts/`, but move real implementations into `linux-lab/scripts/runtime/` and `linux-lab/scripts/network/` so the orchestrator and tests have stable internal paths.

**Tech Stack:** Python 3.12, PyYAML, pytest, GNU make, bash, git

---

## Chunk 1: Script Surface And Catalog Contract

### Task 1: Add failing tests for runtime/network helper parity

**Files:**
- Modify: `tests/test_linux_lab_boot_assets.py`
- Create: `tests/test_linux_lab_runtime_scripts.py`
- Reference: `docs/superpowers/specs/2026-03-14-linux-lab-example-script-parity-design.md`

- [ ] **Step 1: Write failing tests for the required runtime helper surface**

```python
def test_runtime_script_surface_is_ported() -> None:
    expected = [
        ROOT / "linux-lab" / "scripts" / "runtime" / "connect",
        ROOT / "linux-lab" / "scripts" / "runtime" / "guest_run",
        ROOT / "linux-lab" / "scripts" / "runtime" / "guest_insmod",
        ROOT / "linux-lab" / "scripts" / "runtime" / "guest_rmmods",
        ROOT / "linux-lab" / "scripts" / "runtime" / "monitor",
        ROOT / "linux-lab" / "scripts" / "runtime" / "reboot",
        ROOT / "linux-lab" / "scripts" / "runtime" / "qmp",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    assert missing == []
```

- [ ] **Step 2: Write failing tests for the required network helper surface**

```python
def test_network_script_surface_is_ported() -> None:
    expected = [
        ROOT / "linux-lab" / "scripts" / "network" / "up.sh",
        ROOT / "linux-lab" / "scripts" / "network" / "down.sh",
        ROOT / "linux-lab" / "scripts" / "network" / "tap_and_veth_set.sh",
        ROOT / "linux-lab" / "scripts" / "network" / "tap_and_veth_unset.sh",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    assert missing == []
```

- [ ] **Step 3: Write failing tests for top-level compatibility wrappers**

```python
def test_top_level_runtime_wrappers_delegate_to_structured_scripts() -> None:
    wrapper = ROOT / "linux-lab" / "scripts" / "guest_run"
    text = wrapper.read_text(encoding="utf-8")
    assert "scripts/runtime/guest_run" in text
```

- [ ] **Step 4: Run the script parity tests and verify failure**

Run:
```bash
pytest tests/test_linux_lab_runtime_scripts.py -v
```

Expected: FAIL on missing runtime/network helper files and missing top-level wrappers.

- [ ] **Step 5: Commit**

```bash
git add tests/test_linux_lab_runtime_scripts.py tests/test_linux_lab_boot_assets.py
git commit -m "test: add linux-lab runtime script parity coverage"
```

### Task 2: Add failing tests for full example import and catalog coverage

**Files:**
- Create: `tests/test_linux_lab_example_catalog.py`
- Modify: `tests/test_linux_lab_example_assets.py`
- Reference: `docs/superpowers/specs/2026-03-14-linux-lab-example-script-parity-design.md`

- [ ] **Step 1: Write a failing test that compares imported sample directories to `../qulk/modules/`**

```python
def test_imported_example_surface_matches_qulk_module_roots() -> None:
    qulk_modules = sorted(p.name for p in Path("../qulk/modules").iterdir() if p.is_dir())
    catalog = load_example_catalog(ROOT / "linux-lab" / "catalog" / "examples")
    imported = sorted(item["origin"].split("/")[-1] for item in catalog.values())
    assert imported == qulk_modules
```

- [ ] **Step 2: Write failing catalog-schema tests**

```python
def test_catalog_entries_expose_required_fields() -> None:
    item = next(iter(load_example_catalog(CATALOG_ROOT).values()))
    assert set(["key", "kind", "category", "source", "origin", "build_mode", "enabled"]).issubset(item)
```

- [ ] **Step 3: Extend example asset tests to require the full imported surface instead of the curated subset**

```python
def test_catalog_contains_curated_examples_as_enabled_entries() -> None:
    catalog = load_example_catalog(CATALOG_ROOT)
    assert catalog["simple"]["enabled"] is True
    assert catalog["ioctl"]["enabled"] is True
    assert catalog["rust_learn"]["enabled"] is True
```

- [ ] **Step 4: Run the catalog/example tests and verify failure**

Run:
```bash
pytest tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py -v
```

Expected: FAIL on missing catalog module, missing catalog files, and curated-only example assumptions.

- [ ] **Step 5: Commit**

```bash
git add tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py
git commit -m "test: add linux-lab example catalog parity coverage"
```

## Chunk 2: Import Runtime Helpers And Sample Trees

### Task 3: Port runtime and network helper scripts into the new structure

**Files:**
- Create: `linux-lab/scripts/runtime/connect`
- Create: `linux-lab/scripts/runtime/guest_run`
- Create: `linux-lab/scripts/runtime/guest_insmod`
- Create: `linux-lab/scripts/runtime/guest_rmmods`
- Create: `linux-lab/scripts/runtime/monitor`
- Create: `linux-lab/scripts/runtime/reboot`
- Create: `linux-lab/scripts/runtime/qmp`
- Create: `linux-lab/scripts/network/up.sh`
- Create: `linux-lab/scripts/network/down.sh`
- Create: `linux-lab/scripts/network/tap_and_veth_set.sh`
- Create: `linux-lab/scripts/network/tap_and_veth_unset.sh`
- Modify: `linux-lab/scripts/connect`
- Create: `linux-lab/scripts/guest_run`
- Create: `linux-lab/scripts/guest_insmod`
- Create: `linux-lab/scripts/guest_rmmods`
- Create: `linux-lab/scripts/monitor`
- Create: `linux-lab/scripts/reboot`
- Create: `linux-lab/scripts/qmp`
- Test: `tests/test_linux_lab_runtime_scripts.py`

- [ ] **Step 1: Re-run the script parity tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_runtime_scripts.py -v
```

Expected: FAIL

- [ ] **Step 2: Port the required script bodies from `../qulk/scripts/`, rewriting hardcoded host, key, and path assumptions into environment-driven settings**
- [ ] **Step 3: Keep top-level wrappers thin and delegate into `runtime/` or `network/`**
- [ ] **Step 4: Mark imported scripts executable**
- [ ] **Step 5: Re-run the script parity tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_runtime_scripts.py tests/test_linux_lab_boot_assets.py -v
```

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add linux-lab/scripts tests/test_linux_lab_runtime_scripts.py tests/test_linux_lab_boot_assets.py
git commit -m "feat: port linux-lab runtime helper scripts"
```

### Task 4: Import all sample trees and create catalog files

**Files:**
- Create: `linux-lab/catalog/examples/*.yaml`
- Create: `linux-lab/examples/modules/**`
- Create: `linux-lab/examples/userspace/**`
- Create: `linux-lab/examples/rust/**`
- Create: `linux-lab/examples/bpf/**`
- Create: `linux-lab/orchestrator/core/example_catalog.py`
- Modify: `tests/test_linux_lab_example_catalog.py`
- Modify: `tests/test_linux_lab_example_assets.py`

- [ ] **Step 1: Re-run the catalog/example tests and confirm failure**

Run:
```bash
pytest tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py -v
```

Expected: FAIL

- [ ] **Step 2: Import every `../qulk/modules/<name>` tree into the normalized `linux-lab/examples/...` hierarchy, keeping inner sample contents intact**
- [ ] **Step 3: Implement catalog loading and validation in `example_catalog.py`**
- [ ] **Step 4: Create one catalog YAML per imported sample with required fields**
- [ ] **Step 5: Mark the current curated subset as `enabled: true` and the rest as `enabled: false`**
- [ ] **Step 6: Re-run the catalog/example tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py -v
```

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add linux-lab/catalog linux-lab/examples linux-lab/orchestrator/core/example_catalog.py tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py
git commit -m "feat: import linux-lab example corpus and catalog"
```

## Chunk 3: Catalog-Driven Example Planning

### Task 5: Replace hardcoded example planning with catalog-driven resolution

**Files:**
- Modify: `linux-lab/orchestrator/core/examples.py`
- Modify: `linux-lab/orchestrator/stages/build_examples.py`
- Modify: `linux-lab/manifests/profiles/samples.yaml`
- Modify: `linux-lab/manifests/profiles/rust.yaml`
- Modify: `linux-lab/manifests/profiles/bpf.yaml`
- Test: `tests/test_linux_lab_example_catalog.py`
- Test: `tests/test_linux_lab_example_assets.py`
- Test: `tests/test_linux_lab_live_tools_examples.py`

- [ ] **Step 1: Write a focused failing test for catalog-driven plan resolution**

```python
def test_example_planner_uses_catalog_enabled_entries_only() -> None:
    plan = resolve_example_plan(
        example_groups=["modules-core"],
        linux_lab_root=ROOT / "linux-lab",
        build_root=Path("build/linux-lab/request/workspace/build"),
    )
    assert all(item["enabled"] for item in plan[0]["entries"])
```

- [ ] **Step 2: Run the planner tests and verify failure**

Run:
```bash
pytest tests/test_linux_lab_example_catalog.py tests/test_linux_lab_live_tools_examples.py -v
```

Expected: FAIL because the planner still uses embedded path lists.

- [ ] **Step 3: Implement catalog-driven group membership and build-plan derivation in `examples.py`**
- [ ] **Step 4: Update `build_examples.py` to emit discovered-versus-enabled metadata**
- [ ] **Step 5: Keep current curated profile behavior stable through catalog membership**
- [ ] **Step 6: Re-run the planner/example tests and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_live_tools_examples.py -v
```

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add linux-lab/orchestrator/core/examples.py linux-lab/orchestrator/stages/build_examples.py linux-lab/manifests/profiles/samples.yaml linux-lab/manifests/profiles/rust.yaml linux-lab/manifests/profiles/bpf.yaml tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_live_tools_examples.py
git commit -m "feat: switch linux-lab examples to catalog planning"
```

### Task 6: Add integration coverage and docs for the expanded parity surface

**Files:**
- Modify: `README.md`
- Modify: `tests/test_linux_lab_full_plan.py`
- Modify: `tests/test_linux_lab_phase1_repo.py`
- Create: `tests/test_linux_lab_runtime_scripts.py`

- [ ] **Step 1: Extend integration tests to assert the presence of catalog metadata and script helper metadata in the full run plan**
- [ ] **Step 2: Document the expanded example catalog and helper scripts in `README.md`**
- [ ] **Step 3: Run the integrated linux-lab slice and confirm pass**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_runtime_scripts.py tests/test_linux_lab_full_plan.py tests/test_linux_lab_boot_assets.py -v
```

Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add README.md tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_example_catalog.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_runtime_scripts.py tests/test_linux_lab_full_plan.py tests/test_linux_lab_boot_assets.py
git commit -m "test: cover linux-lab example and script parity"
```

## Chunk 4: Final Verification

### Task 7: Run the full linux-lab verification stack and capture parity status

**Files:**
- Modify: `README.md` if verification reveals missing operator notes

- [ ] **Step 1: Run the full linux-lab pytest slice**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_example_catalog.py tests/test_linux_lab_full_plan.py tests/test_linux_lab_live_execution.py tests/test_linux_lab_live_tools_examples.py tests/test_linux_lab_boot_assets.py tests/test_linux_lab_runtime_scripts.py -v
```

Expected: PASS

- [ ] **Step 2: Run the existing smoke commands**

Run:
```bash
make linux-lab-run
linux-lab/bin/linux-lab run --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab --dry-run
linux-lab/bin/linux-lab run --kernel 6.18.4 --arch arm64 --image noble --profile minimal --dry-run --stop-after boot
linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg
```

Expected: all commands exit `0`

- [ ] **Step 3: Commit any last documentation or test-only fixes**

```bash
git add README.md tests
git commit -m "docs: finalize linux-lab parity verification"
```

