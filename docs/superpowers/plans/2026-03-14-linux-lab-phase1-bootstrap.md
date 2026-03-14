# Linux Lab Phase 1 Bootstrap Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the `linux-lab` Phase 1 scaffold: native CLI, `ulk` compatibility shim, manifest loading and validation, placeholder stage planning, and root-level repo integration.

**Architecture:** Phase 1 is planning-only. The implementation adds a small Python orchestrator under `linux-lab/orchestrator/`, schema-valid YAML manifests under `linux-lab/manifests/`, two CLI entrypoints under `linux-lab/bin/`, and pytest guards that prove request resolution and placeholder stage-state emission work end to end. Root `Makefile`, `README.md`, and `.gitignore` integrate the subsystem without changing existing targets.

**Tech Stack:** Python 3.12, PyYAML, pytest, GNU Make, repository-root shell entrypoints

---

## Chunk 1: Phase 1 Bootstrap

### Task 1: Add Failing Phase 1 Guard Tests

**Files:**
- Create: `tests/test_linux_lab_phase1_repo.py`
- Create: `tests/test_linux_lab_phase1_resolution.py`
- Create: `tests/test_linux_lab_phase1_plan.py`
- Reference: `docs/superpowers/specs/2026-03-14-linux-lab-design.md`

- [ ] **Step 1: Write the failing repo integration tests**

```python
def test_linux_lab_repo_layout_exists() -> None:
    expected = [
        "linux-lab/bin/linux-lab",
        "linux-lab/bin/ulk",
        "linux-lab/orchestrator/cli.py",
        "linux-lab/manifests/kernels/6.18.4.yaml",
        "linux-lab/manifests/arches/x86_64.yaml",
    ]
```

- [ ] **Step 2: Write the failing manifest and compatibility tests**

```python
def test_ulk_parser_rejects_duplicate_keys() -> None:
    result = _run(["linux-lab/bin/ulk", "arch=x86_64", "arch=arm64"])
    assert result.returncode != 0
    assert "duplicate" in result.stderr.lower()
```

- [ ] **Step 3: Write the failing plan-state tests**

```python
def test_plan_writes_request_and_placeholder_stage_state(tmp_path: Path) -> None:
    result = _run_cli("plan", "--kernel", "6.18.4", "--arch", "x86_64", "--image", "noble", "--profile", "default-lab")
    assert result.returncode == 0
    assert (ROOT / "build/linux-lab/requests").is_dir()
```

- [ ] **Step 4: Run test files to verify they fail**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py -v
```

Expected: FAIL with missing files, missing Make targets, and missing `linux-lab` CLI behavior.

- [ ] **Step 5: Commit**

```bash
git add tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py
git commit -m "test: add linux-lab phase1 guard coverage"
```

### Task 2: Implement Manifest Loading and Request Resolution

**Files:**
- Create: `linux-lab/orchestrator/__init__.py`
- Create: `linux-lab/orchestrator/core/__init__.py`
- Create: `linux-lab/orchestrator/core/manifests.py`
- Create: `linux-lab/orchestrator/core/request.py`
- Create: `linux-lab/manifests/kernels/6.18.4.yaml`
- Create: `linux-lab/manifests/kernels/4.19.317.yaml`
- Create: `linux-lab/manifests/kernels/6.4.3.yaml`
- Create: `linux-lab/manifests/kernels/6.9.6.yaml`
- Create: `linux-lab/manifests/kernels/6.9.8.yaml`
- Create: `linux-lab/manifests/kernels/6.10.yaml`
- Create: `linux-lab/manifests/arches/x86_64.yaml`
- Create: `linux-lab/manifests/arches/arm64.yaml`
- Create: `linux-lab/manifests/images/noble.yaml`
- Create: `linux-lab/manifests/images/jammy.yaml`
- Create: `linux-lab/manifests/profiles/debug.yaml`
- Create: `linux-lab/manifests/profiles/bpf.yaml`
- Create: `linux-lab/manifests/profiles/rust.yaml`
- Create: `linux-lab/manifests/profiles/samples.yaml`
- Create: `linux-lab/manifests/profiles/debug-tools.yaml`
- Create: `linux-lab/manifests/profiles/minimal.yaml`
- Create: `linux-lab/manifests/profiles/default-lab.yaml`
- Test: `tests/test_linux_lab_phase1_resolution.py`

- [ ] **Step 1: Implement the failing manifest loader test target first**

```python
def test_default_compat_is_unique_per_manifest_family() -> None:
    manifests = load_manifests(ROOT / "linux-lab" / "manifests")
    assert manifests.default_kernel == "6.18.4"
    assert manifests.default_image == "noble"
```

- [ ] **Step 2: Run the resolution test to verify it fails**

Run:
```bash
pytest tests/test_linux_lab_phase1_resolution.py -v
```

Expected: FAIL with import errors or missing manifests/fields.

- [ ] **Step 3: Write minimal manifest and request-resolution code**

```python
request = resolve_request(
    manifests=manifests,
    command="plan",
    kernel="6.18.4",
    arch="x86_64",
    image="noble",
    profiles=["default-lab"],
    mirror=None,
    compat_mode=False,
)
```

Implementation requirements:
- use PyYAML to load `.yaml` files
- validate the Phase 1 minimum schema fields from the spec
- resolve `default-lab` into ordered concrete profiles
- reject malformed compatibility args, duplicates, invalid mirrors, and missing required profiles
- produce a deterministic `request_fingerprint` and `artifact_root`

- [ ] **Step 4: Run the resolution tests to verify they pass**

Run:
```bash
pytest tests/test_linux_lab_phase1_resolution.py -v
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add linux-lab/orchestrator/__init__.py linux-lab/orchestrator/core/__init__.py linux-lab/orchestrator/core/manifests.py linux-lab/orchestrator/core/request.py linux-lab/manifests tests/test_linux_lab_phase1_resolution.py
git commit -m "feat: add linux-lab phase1 manifest resolution"
```

### Task 3: Implement Native CLI, `ulk` Shim, and Placeholder Stage Planning

**Files:**
- Create: `linux-lab/bin/linux-lab`
- Create: `linux-lab/bin/ulk`
- Create: `linux-lab/orchestrator/cli.py`
- Create: `linux-lab/orchestrator/core/state.py`
- Create: `linux-lab/orchestrator/core/stages.py`
- Create: `linux-lab/orchestrator/stages/__init__.py`
- Create: `linux-lab/orchestrator/stages/fetch.py`
- Create: `linux-lab/orchestrator/stages/prepare.py`
- Create: `linux-lab/orchestrator/stages/patch.py`
- Create: `linux-lab/orchestrator/stages/configure.py`
- Create: `linux-lab/orchestrator/stages/build_kernel.py`
- Create: `linux-lab/orchestrator/stages/build_tools.py`
- Create: `linux-lab/orchestrator/stages/build_examples.py`
- Create: `linux-lab/orchestrator/stages/build_image.py`
- Create: `linux-lab/orchestrator/stages/boot.py`
- Test: `tests/test_linux_lab_phase1_plan.py`

- [ ] **Step 1: Extend the plan-state test with concrete state-file assertions**

```python
def test_plan_writes_validate_and_stage_json_files() -> None:
    request_root = _request_root_for(...)
    assert (request_root / "state" / "validate.json").is_file()
    assert (request_root / "state" / "prepare.json").is_file()
    assert (request_root / "state" / "boot.json").is_file()
```

- [ ] **Step 2: Run the plan-state tests to verify they fail**

Run:
```bash
pytest tests/test_linux_lab_phase1_plan.py -v
```

Expected: FAIL with missing CLI entrypoints and missing state files.

- [ ] **Step 3: Write the minimal CLI and stage runtime**

```python
def main(argv: list[str] | None = None) -> int:
    args = parser.parse_args(argv)
    if args.command == "validate":
        request = resolve_native_request(...)
        return 0
    request = resolve_native_request(...)
    emit_plan_state(request)
    return 0
```

Implementation requirements:
- `linux-lab/bin/linux-lab` dispatches native `validate` and `plan`
- `linux-lab/bin/ulk` accepts only `key=value` args and forwards to native `plan`
- `validate` is read-only
- `plan` writes `request.json`, `state/validate.json`, all `<stage>.json` files, and referenced log paths under `build/linux-lab/requests/<fingerprint>/`
- `prepare` performs only local writable-path and Python-runtime checks
- placeholder stages emit canonical consumed/produced artifact names from the spec

- [ ] **Step 4: Run the plan-state tests to verify they pass**

Run:
```bash
pytest tests/test_linux_lab_phase1_plan.py -v
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add linux-lab/bin/linux-lab linux-lab/bin/ulk linux-lab/orchestrator/cli.py linux-lab/orchestrator/core/state.py linux-lab/orchestrator/core/stages.py linux-lab/orchestrator/stages tests/test_linux_lab_phase1_plan.py
git commit -m "feat: add linux-lab phase1 planning runtime"
```

### Task 4: Integrate Root Makefile, README, and Ignore Rules

**Files:**
- Modify: `Makefile`
- Modify: `README.md`
- Modify: `.gitignore`
- Test: `tests/test_linux_lab_phase1_repo.py`

- [ ] **Step 1: Add failing root-integration assertions**

```python
def test_linux_lab_make_targets_and_docs_exist() -> None:
    makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    ignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
    assert "linux-lab-validate:" in makefile
    assert "linux-lab-plan:" in makefile
    assert "linux-lab/" in readme
    assert "build/linux-lab/" in ignore
```

- [ ] **Step 2: Run the repo test to verify it fails**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py -v
```

Expected: FAIL with missing Make targets, README section, and ignore rule.

- [ ] **Step 3: Write the minimal root integration**

```make
LINUX_LAB_KERNEL ?= 6.18.4
LINUX_LAB_ARCH ?= x86_64
LINUX_LAB_IMAGE ?= noble
LINUX_LAB_PROFILES ?= default-lab
```

Implementation requirements:
- add `linux-lab-validate` and `linux-lab-plan` root targets
- keep existing targets unchanged
- add a short README Phase 1 note with example commands
- ignore `build/linux-lab/`

- [ ] **Step 4: Run the repo tests and then the full new Phase 1 suite**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py -v
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py -v
```

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add Makefile README.md .gitignore tests/test_linux_lab_phase1_repo.py
git commit -m "feat: integrate linux-lab phase1 entrypoints"
```

### Final Verification

**Files:**
- Test: `tests/test_linux_lab_phase1_repo.py`
- Test: `tests/test_linux_lab_phase1_resolution.py`
- Test: `tests/test_linux_lab_phase1_plan.py`

- [ ] **Step 1: Run the complete targeted Phase 1 verification**

Run:
```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py -v
```

Expected: all tests PASS

- [ ] **Step 2: Run a regression check on existing root integration**

Run:
```bash
pytest tests/test_linux1_make_targets.py tests/test_repo_layout.py -v
```

Expected: PASS

- [ ] **Step 3: Summarize results and note deferred work**

Deferred to later phases:
- real patch asset migration
- real config fragments
- real image recipes
- real boot/runtime execution
