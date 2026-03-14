# Linux Lab ULK Tooling Parity Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend `linux-lab` so it covers the standalone tool and repo-local userspace surface that `../qulk/ulk` actually uses: external tool manifests, kernel in-tree tool planning and execution, `crash` extension assets, and `rust_learn/user`.

**Architecture:** Keep the current stage ownership intact. `build-tools` will gain explicit support for manifest `build_policy`, `post_prepare_asset_copies`, and kernel in-tree tool planning while `build-examples` continues to own `rust_learn/user`. New metadata is additive, and downstream stages remain behaviorally unchanged.

**Tech Stack:** Python 3.12, YAML manifests, pytest, bash helper assets, git-managed fixture repos

---

## File Map

**Modify:**
- `linux-lab/orchestrator/core/tooling.py`  
  Responsibility: validate and resolve external tool manifests plus kernel in-tree tool planning metadata.
- `linux-lab/orchestrator/stages/build_tools.py`  
  Responsibility: translate resolved tool and kernel-tool plans into dry-run metadata and live execution.
- `linux-lab/tools/crash.yaml`
- `linux-lab/tools/cgdb.yaml`
- `linux-lab/tools/libbpf-bootstrap.yaml`
- `linux-lab/tools/retsnoop.yaml`  
  Responsibility: declare exact upstream tool behavior with `build_policy`, prepare/build commands, and asset-copy metadata.
- `linux-lab/manifests/kernels/4.19.317.yaml`
- `linux-lab/manifests/kernels/6.4.3.yaml`
- `linux-lab/manifests/kernels/6.9.6.yaml`
- `linux-lab/manifests/kernels/6.9.8.yaml`
- `linux-lab/manifests/kernels/6.10.yaml`
- `linux-lab/manifests/kernels/6.18.4.yaml`  
  Responsibility: add the `kernel-tools` group to the supported kernel manifests.
- `tests/test_linux_lab_tool_assets.py`  
  Responsibility: dry-run and planner coverage for manifest fields, external tools, and kernel-tool metadata.
- `tests/test_linux_lab_example_assets.py`  
  Responsibility: dry-run regression coverage for catalog-driven example planning, including `rust_learn`.
- `tests/test_linux_lab_live_tools_examples.py`  
  Responsibility: fixture-backed live execution coverage for host-build, guest-build, asset-copy, kernel-tools, and repo-local userspace behavior.

**Create:**
- `linux-lab/tools/assets/crash/extensions/Makefile`
- `linux-lab/tools/assets/crash/extensions/dminfo.c`
- `linux-lab/tools/assets/crash/extensions/echo.c`
- `linux-lab/tools/assets/crash/extensions/eppic.c`
- `linux-lab/tools/assets/crash/extensions/eppic.mk`
- `linux-lab/tools/assets/crash/extensions/snap.c`
- `linux-lab/tools/assets/crash/extensions/snap.mk`  
  Responsibility: repo-local `crash` extension assets copied into the checked-out `crash` tree during tool preparation.

**Reference:**
- `docs/superpowers/specs/2026-03-14-linux-lab-ulk-tooling-parity-design.md`
- `linux-lab/catalog/examples/rust_learn.yaml`
- `linux-lab/orchestrator/stages/build_examples.py`

## Chunk 1: Manifest Schema And Dry-Run Planning

### Task 1: Add failing tests for manifest schema and dry-run metadata

**Files:**
- Modify: `tests/test_linux_lab_tool_assets.py`
- Modify: `tests/test_linux_lab_example_assets.py`
- Reference: `linux-lab/tools/*.yaml`
- Reference: `linux-lab/manifests/kernels/*.yaml`

- [ ] **Step 1: Write the failing tests**

Add tests that assert:
- `crash.yaml` has `build_policy: "host-build"` and `post_prepare_asset_copies` pointing at `linux-lab/tools/assets/crash/extensions/`
- `cgdb.yaml` has `build_policy: "host-build"`
- `libbpf-bootstrap.yaml` and `retsnoop.yaml` have `build_policy: "guest-build"`
- malformed tool manifests such as `build_policy: "wrong"` fail during planning
- every kernel manifest listed above includes `kernel-tools` in `tool_groups`
- dry-run `build-tools.json` exposes:
  - `external_tools`
  - `kernel_tools`
  - `post_prepare_asset_copies`
  - legacy `tools` alias for backward compatibility in this pass
  - per-tool `build_policy`
  - per-tool `checkout_dir`
  - per-tool `prepare_commands`
  - per-tool `build_commands`
  - per-tool `guest_build_commands`
  - per-tool `post_prepare_asset_copies`
- `kernel_tools` contains the exact planned `ulk` commands for `tools/libapi` and `make -C <kernel>/tools ... subdir=tools all`
- dry-run `build-examples.json` still includes the catalog-driven `rust_learn` entry for a `default-lab` request
- minimal profile still yields empty tool metadata

- [ ] **Step 2: Run the focused test file to verify it fails**

Run: `pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py -v`
Expected: FAIL on missing manifest fields, missing `kernel_tools` dry-run metadata, or missing preserved `rust_learn` planning assertions.

- [ ] **Step 3: Update the external tool manifests**

Modify:
- `linux-lab/tools/crash.yaml`
- `linux-lab/tools/cgdb.yaml`
- `linux-lab/tools/libbpf-bootstrap.yaml`
- `linux-lab/tools/retsnoop.yaml`

Implementation notes:
- add `build_policy` to each manifest
- add `post_prepare_asset_copies` to `crash.yaml`
- add `guest_build_commands` where needed for guest-built tools

- [ ] **Step 4: Update the kernel manifest gating**

Modify:
- `linux-lab/manifests/kernels/4.19.317.yaml`
- `linux-lab/manifests/kernels/6.4.3.yaml`
- `linux-lab/manifests/kernels/6.9.6.yaml`
- `linux-lab/manifests/kernels/6.9.8.yaml`
- `linux-lab/manifests/kernels/6.10.yaml`
- `linux-lab/manifests/kernels/6.18.4.yaml`

Implementation notes:
- add `kernel-tools` to `tool_groups` for each manifest

- [ ] **Step 5: Implement the planning and metadata changes**

Modify:
- `linux-lab/orchestrator/core/tooling.py`
- `linux-lab/orchestrator/stages/build_tools.py`

Implementation notes:
- extend the manifest loader to validate and normalize `build_policy`, `post_prepare_asset_copies`, and `guest_build_commands`
- keep `resolve_tool_keys()` focused on external tools
- add a dedicated kernel-tool resolver in `tooling.py` that emits the exact two `ulk` commands:
  - `make O=<build_root> ARCH=<arch> CROSS_COMPILE=<prefix> tools/libapi`
  - `make -C <kernel_tree>/tools O=<build_root> subdir=tools all`
- make `build-tools` dry-run metadata emit `external_tools`, `kernel_tools`, and aggregate `post_prepare_asset_copies`
- preserve the existing `tools` metadata key as a backward-compatible alias during this pass so downstream state readers remain unchanged while the new additive keys land

- [ ] **Step 6: Run the focused test file to verify it passes**

Run: `pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py -v`
Expected: PASS

- [ ] **Step 7: Run the directly related dry-run CLI checks**

Run: `rm -rf build/linux-lab/requests`
Expected: no output

Run: `linux-lab/bin/linux-lab run --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab --dry-run`
Expected: exit `0`

Run:
```bash
python3 - <<'PY'
from pathlib import Path
requests = sorted(Path('build/linux-lab/requests').glob('*'))
assert requests, 'missing default-lab request dir'
Path('build/linux-lab/default-request-root.txt').write_text(str(requests[-1]), encoding='utf-8')
print(requests[-1])
PY
```
Expected: prints the default-lab request root path

Run: `linux-lab/bin/linux-lab run --kernel 6.18.4 --arch arm64 --image noble --profile minimal --dry-run`
Expected: exit `0`

Run:
```bash
python3 - <<'PY'
from pathlib import Path
requests = sorted(Path('build/linux-lab/requests').glob('*'))
assert len(requests) >= 2, 'missing minimal request dir'
known = Path('build/linux-lab/default-request-root.txt').read_text(encoding='utf-8').strip()
latest = max((str(path) for path in requests if str(path) != known), key=len)
Path('build/linux-lab/minimal-request-root.txt').write_text(latest, encoding='utf-8')
print(latest)
PY
```
Expected: prints the minimal request root path

- [ ] **Step 8: Assert the emitted state shape explicitly**

Run:
```bash
python3 - <<'PY'
import json
from pathlib import Path
default_root = Path('build/linux-lab/default-request-root.txt').read_text(encoding='utf-8').strip()
minimal_root = Path('build/linux-lab/minimal-request-root.txt').read_text(encoding='utf-8').strip()
default_state = json.loads((Path(default_root) / 'state' / 'build-tools.json').read_text())
minimal_state = json.loads((Path(minimal_root) / 'state' / 'build-tools.json').read_text())
assert 'tools' in default_state['metadata']
assert 'external_tools' in default_state['metadata']
assert 'kernel_tools' in default_state['metadata']
assert 'post_prepare_asset_copies' in default_state['metadata']
for item in default_state['metadata']['external_tools']:
    for key in ('key', 'build_policy', 'checkout_dir', 'prepare_commands', 'build_commands', 'guest_build_commands', 'post_prepare_asset_copies'):
        assert key in item, (item['key'], key)
commands = [' '.join(item['command']) for item in default_state['metadata']['kernel_tools']]
assert any('tools/libapi' in command for command in commands), commands
assert any('subdir=tools all' in command for command in commands), commands
assert minimal_state['metadata']['tools'] == []
assert minimal_state['metadata']['external_tools'] == []
assert minimal_state['metadata']['kernel_tools'] == []
print('build-tools metadata shape ok')
PY
```
Expected: prints `build-tools metadata shape ok`

- [ ] **Step 9: Run the directly related regression slice**

Run: `pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_example_catalog.py tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_full_plan.py -v`
Expected: PASS

- [ ] **Step 10: Commit the planning/schema slice**

```bash
git add \
  linux-lab/tools/crash.yaml \
  linux-lab/tools/cgdb.yaml \
  linux-lab/tools/libbpf-bootstrap.yaml \
  linux-lab/tools/retsnoop.yaml \
  linux-lab/manifests/kernels/4.19.317.yaml \
  linux-lab/manifests/kernels/6.4.3.yaml \
  linux-lab/manifests/kernels/6.9.6.yaml \
  linux-lab/manifests/kernels/6.9.8.yaml \
  linux-lab/manifests/kernels/6.10.yaml \
  linux-lab/manifests/kernels/6.18.4.yaml \
  linux-lab/orchestrator/core/tooling.py \
  linux-lab/orchestrator/stages/build_tools.py \
  tests/test_linux_lab_tool_assets.py \
  tests/test_linux_lab_example_assets.py
git commit -m "feat: plan linux-lab ulk tool parity"
```

## Chunk 2: Live Execution And Crash Asset Import

### Task 2: Add failing live-execution tests for asset copy, guest-build host skip, and kernel-tools

**Files:**
- Modify: `tests/test_linux_lab_live_tools_examples.py`
- Reference: `linux-lab/orchestrator/stages/build_tools.py`

- [ ] **Step 1: Write the failing tests**

Add fixture-backed tests that assert:
- host-built `crash` copies the repo-local assets into `<checkout>/extensions/`
- guest-built `libbpf-bootstrap` and `retsnoop` run clone and prepare only, and do not run `build_commands` on the host
- guest-built tool stage state still records `build_policy: guest-build` and non-empty `guest_build_commands`
- kernel-tool planning commands execute in the selected request workspace and leave a deterministic file marker
- `build-examples` still builds `rust_learn/user` from the example-owned path
- missing `crash` extension assets fail the stage with an inspectable log and `build-tools.json` error state
- missing kernel workspace paths fail before kernel-tool command execution and still leave inspectable log and state output
- failed `build-tools.json` records `status: "failed"`, a concrete `error_kind`, and a specific `error_message`
- missing `crash` assets report a specific message of the form `missing crash extension asset source: <path>`
- missing kernel workspace reports a specific message of the form `missing kernel workspace: <path>`

Suggested fixture pattern:
- write tool manifests into a temporary `labroot`
- use small local git repos for external tools
- use shell commands such as `printf` and `cp -a` so the tests assert behavior without requiring real network or kernel source

- [ ] **Step 2: Run the focused live test file to verify it fails**

Run: `pytest tests/test_linux_lab_live_tools_examples.py -v`
Expected: FAIL on missing asset-copy execution, missing guest-build host skip, or missing kernel-tool execution.

- [ ] **Step 3: Import the crash asset files**

Create:
- `linux-lab/tools/assets/crash/extensions/Makefile`
- `linux-lab/tools/assets/crash/extensions/dminfo.c`
- `linux-lab/tools/assets/crash/extensions/echo.c`
- `linux-lab/tools/assets/crash/extensions/eppic.c`
- `linux-lab/tools/assets/crash/extensions/eppic.mk`
- `linux-lab/tools/assets/crash/extensions/snap.c`
- `linux-lab/tools/assets/crash/extensions/snap.mk`

Source of truth for import: `../qulk/crash/extensions/`

- [ ] **Step 4: Implement the minimal live-execution changes**

Modify:
- `linux-lab/orchestrator/core/tooling.py`
- `linux-lab/orchestrator/stages/build_tools.py`

Implementation notes:
- perform `post_prepare_asset_copies` after `prepare_commands`
- enforce `build_policy`
  - `host-build`: run `build_commands`
  - `guest-build`: skip host `build_commands`, but keep `guest_build_commands` in metadata
- execute `kernel_tools` after external tool preparation/build
- keep logging through `run_stage_command()` so failures remain stage-scoped and inspectable
- `build_tools.py` must catch its own asset-copy, workspace, and command failures and return a failed stage result instead of letting the exception abort state writing
- the failed stage result must set:
  - `status: "failed"`
  - `error_kind`: `missing-asset`, `missing-kernel-workspace`, or `command-failed`
  - `error_message`: the specific message asserted by the new tests

- [ ] **Step 5: Run the focused live test file to verify it passes**

Run: `pytest tests/test_linux_lab_live_tools_examples.py -v`
Expected: PASS

- [ ] **Step 6: Run the combined regression slice**

Run:
`pytest tests/test_linux_lab_tool_assets.py tests/test_linux_lab_live_tools_examples.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_live_execution.py tests/test_linux_lab_runtime_scripts.py -v`

Expected: all selected tests PASS.

- [ ] **Step 7: Commit the live-execution slice**

```bash
git add \
  linux-lab/tools/assets/crash/extensions/Makefile \
  linux-lab/tools/assets/crash/extensions/dminfo.c \
  linux-lab/tools/assets/crash/extensions/echo.c \
  linux-lab/tools/assets/crash/extensions/eppic.c \
  linux-lab/tools/assets/crash/extensions/eppic.mk \
  linux-lab/tools/assets/crash/extensions/snap.c \
  linux-lab/tools/assets/crash/extensions/snap.mk \
  linux-lab/orchestrator/core/tooling.py \
  linux-lab/orchestrator/stages/build_tools.py \
  tests/test_linux_lab_live_tools_examples.py
git commit -m "feat: execute linux-lab ulk tool workflows"
```

## Chunk 3: Full Verification

### Task 3: Run the full linux-lab verification slice and record outcomes

**Files:**
- No code changes required unless verification exposes a regression
- Reference: `docs/superpowers/specs/2026-03-14-linux-lab-ulk-tooling-parity-design.md`

- [ ] **Step 1: Run the full relevant pytest slice**

Run:
`pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py tests/test_linux_lab_phase2_assets.py tests/test_linux_lab_phase2_kernel_assets.py tests/test_linux_lab_runtime_cli.py tests/test_linux_lab_image_assets.py tests/test_linux_lab_tool_assets.py tests/test_linux_lab_example_assets.py tests/test_linux_lab_example_catalog.py tests/test_linux_lab_full_plan.py tests/test_linux_lab_live_execution.py tests/test_linux_lab_live_tools_examples.py tests/test_linux_lab_boot_assets.py tests/test_linux_lab_runtime_scripts.py -v`

Expected: all selected tests PASS.

- [ ] **Step 2: Run the user-facing command checks**

Run: `make linux-lab-run`
Expected: exit `0`

Run:
```bash
python3 - <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, str(Path('linux-lab').resolve()))
from orchestrator.core.manifests import load_manifests
from orchestrator.core.request import resolve_native_request
manifests = load_manifests(Path('linux-lab/manifests'))
request = resolve_native_request(
    manifests=manifests,
    kernel='6.18.4',
    arch='x86_64',
    image='noble',
    profiles=['default-lab'],
    mirror=None,
    command='run',
    dry_run=True,
)
Path('build/linux-lab/final-native-request-root.txt').write_text(str(request.artifact_root), encoding='utf-8')
print(request.artifact_root)
PY
```
Expected: prints the exact native dry-run `artifact_root`

Run: `linux-lab/bin/linux-lab run --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab --dry-run`
Expected: exit `0`

Run: `linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg`
Expected: exit `0`

- [ ] **Step 3: Assert the end-to-end dry-run metadata contract**

Run:
```bash
python3 - <<'PY'
import json
from pathlib import Path
request_root = Path(Path('build/linux-lab/final-native-request-root.txt').read_text(encoding='utf-8').strip())
tool_state = json.loads((request_root / 'state' / 'build-tools.json').read_text())
example_state = json.loads((request_root / 'state' / 'build-examples.json').read_text())
assert 'external_tools' in tool_state['metadata']
assert 'kernel_tools' in tool_state['metadata']
assert any(item['key'] == 'crash' for item in tool_state['metadata']['external_tools'])
assert any('tools/libapi' in ' '.join(item['command']) for item in tool_state['metadata']['kernel_tools'])
assert any(entry['key'] == 'rust_learn' for plan in example_state['metadata']['example_plans'] for entry in plan['entries'])
print(request_root)
print('end-to-end metadata contract ok')
PY
```
Expected: prints the request root and `end-to-end metadata contract ok`

- [ ] **Step 4: Inspect the working tree**

Run: `git status --short`
Expected: no unexpected uncommitted changes before the final review step.

- [ ] **Step 5: Commit only if verification forced a follow-up fix**

```bash
git add -A
git commit -m "test: lock linux-lab ulk tooling parity coverage"
```

Use this step only if one of the commands above failed, you made a bounded fix, and you reran the failing verification command to green before committing.
