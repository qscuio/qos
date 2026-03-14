# Repo Ownership Reorg Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reorganize Linux 1.0.0-owned repo content under `linux-1.0.0/`, regroup Linux1 and xv6 scripts under owner-scoped subdirectories, and preserve the current top-level `make` entrypoints.

**Architecture:** This is a compatibility-preserving path reorg, not a behavior redesign. The work starts by making the current tests fail on the old flat layout, then moves owned assets with `git mv`, updates path resolution in scripts and Makefile recipes, and finishes by updating docs plus a narrow verification slice that proves Linux1, xv6, and repo-layout entrypoints still line up.

**Tech Stack:** Bash, GNU Make, pytest, git file moves, Markdown docs

---

## Chunk 1: Lock In The New Ownership Contract With Failing Tests

### Task 1: Update Repo Layout And Target Path Assertions

**Files:**
- Modify: `tests/test_repo_layout.py`
- Modify: `tests/test_linux1_make_targets.py`
- Modify: `tests/test_xv6.py`

- [ ] **Step 1: Write the failing path assertions**

Update `tests/test_repo_layout.py` to assert the Linux1-owned layout explicitly:

```python
EXPECTED_OS_ROOTS = [
    "c-os",
    "rust-os",
    "linux-1.0.0",
    "linux-lab",
    "xv6",
]

LINUX1_OWNED_PATHS = [
    "linux-1.0.0/userspace/src/init.S",
    "linux-1.0.0/manifests/linux1-sources.lock",
    "linux-1.0.0/patches/lilo",
]

REMOVED_TOP_LEVEL_PATHS = [
    "linux1-userspace",
]
```

Update `tests/test_linux1_make_targets.py` to point at:

```python
CURSES_RUNNER = ROOT / "scripts" / "linux1" / "run_curses.sh"
```

Update `tests/test_xv6.py` so the fixture builds through:

```python
["bash", "scripts/xv6/build.sh"]
```

- [ ] **Step 2: Run the targeted tests to verify they fail on old paths**

Run:

```bash
pytest tests/test_repo_layout.py tests/test_linux1_make_targets.py tests/test_xv6.py -v
```

Expected: FAIL with missing `linux-1.0.0/userspace/...`, missing `scripts/linux1/run_curses.sh`, and missing `scripts/xv6/build.sh` assertions.

- [ ] **Step 3: Commit the test-only contract change**

```bash
git add tests/test_repo_layout.py tests/test_linux1_make_targets.py tests/test_xv6.py
git commit -m "test: lock repo ownership path contract"
```

### Task 2: Update Linux1 Test Entry Points To New Script And Asset Paths

**Files:**
- Modify: `tests/test_linux1.py`
- Modify: `tests/test_linux1_userspace_layout.py`
- Modify: `tests/test_linux1_kernel_layout.py`
- Modify: `tests/test_linux1_disk_geometry.py`

- [ ] **Step 1: Write the failing Linux1 path updates**

Change direct script invocations from:

```python
["bash", "scripts/build_linux1_kernel.sh"]
["bash", "scripts/build_linux1_userspace.sh"]
run_step("bash", "scripts/mk_linux1_disk.sh")
```

to:

```python
["bash", "scripts/linux1/build_kernel.sh"]
["bash", "scripts/linux1/build_userspace.sh"]
run_step("bash", "scripts/linux1/mk_disk.sh")
```

Keep build artifacts under `build/linux1/...` unchanged. Add explicit layout assertions where useful:

```python
USPACE_SRC = ROOT / "linux-1.0.0" / "userspace" / "src"
MANIFEST = ROOT / "linux-1.0.0" / "manifests" / "linux1-sources.lock"
```

- [ ] **Step 2: Run the Linux1 path tests to verify they fail**

Run:

```bash
pytest \
  tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py \
  tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py -v
```

Expected: FAIL because `scripts/linux1/...` does not exist yet and the Linux1-owned asset paths are not moved yet.

- [ ] **Step 3: Commit the Linux1 test updates**

```bash
git add tests/test_linux1.py tests/test_linux1_userspace_layout.py tests/test_linux1_kernel_layout.py tests/test_linux1_disk_geometry.py
git commit -m "test: repoint linux1 coverage to owned paths"
```

## Chunk 2: Move Owned Assets And Repoint The Build Surface

### Task 3: Move Linux1-Owned Assets Under `linux-1.0.0/`

**Files:**
- Create: `linux-1.0.0/userspace/`
- Create: `linux-1.0.0/manifests/`
- Create: `linux-1.0.0/patches/kernel/`
- Create: `linux-1.0.0/patches/lilo/`
- Move: `linux1-userspace/README.md`
- Move: `linux1-userspace/src/common.inc`
- Move: `linux1-userspace/src/echo.S`
- Move: `linux1-userspace/src/init.S`
- Move: `linux1-userspace/src/sh.S`
- Move: `linux1-userspace/src/syscalls.S`
- Move: `manifests/linux1-sources.lock`
- Move: `patches/lilo/*.patch`

- [ ] **Step 1: Move the owned directories with git-preserving renames**

Run:

```bash
mkdir -p linux-1.0.0/userspace linux-1.0.0/manifests linux-1.0.0/patches
git mv linux1-userspace/README.md linux-1.0.0/userspace/README.md
git mv linux1-userspace/src linux-1.0.0/userspace/src
git mv manifests/linux1-sources.lock linux-1.0.0/manifests/linux1-sources.lock
git mv patches/lilo linux-1.0.0/patches/lilo
```

If `patches/linux-1.0.0/` exists, move it to `linux-1.0.0/patches/kernel/` with `git mv`. If it does not exist, do not create fake placeholder patch files.

- [ ] **Step 2: Verify the moved ownership layout exists**

Run:

```bash
test -f linux-1.0.0/userspace/src/init.S
test -f linux-1.0.0/manifests/linux1-sources.lock
test -d linux-1.0.0/patches/lilo
test ! -e linux1-userspace
```

Expected: all commands succeed.

- [ ] **Step 3: Commit the asset move**

```bash
git add linux-1.0.0/manifests linux-1.0.0/patches linux-1.0.0/userspace
git commit -m "refactor: move linux1-owned assets under linux-1.0.0"
```

### Task 4: Regroup Linux1 And xv6 Scripts Under Owner Directories

**Files:**
- Create: `scripts/linux1/`
- Create: `scripts/xv6/`
- Move: `scripts/fetch_linux1_sources.sh`
- Move: `scripts/build_linux1_kernel.sh`
- Move: `scripts/build_linux1_userspace.sh`
- Move: `scripts/build_linux1_lilo.sh`
- Move: `scripts/mk_linux1_disk.sh`
- Move: `scripts/run_linux1_qemu.sh`
- Move: `scripts/run_linux1_curses.sh`
- Move: `scripts/verify_linux1_provenance.sh`
- Move: `scripts/linux1_userspace.ld`
- Move: `scripts/linux1_zsystem.ld`
- Move: `scripts/build_xv6.sh`

- [ ] **Step 1: Move owner-scoped scripts and linker assets**

Run:

```bash
mkdir -p scripts/linux1 scripts/xv6
git mv scripts/fetch_linux1_sources.sh scripts/linux1/fetch_sources.sh
git mv scripts/build_linux1_kernel.sh scripts/linux1/build_kernel.sh
git mv scripts/build_linux1_userspace.sh scripts/linux1/build_userspace.sh
git mv scripts/build_linux1_lilo.sh scripts/linux1/build_lilo.sh
git mv scripts/mk_linux1_disk.sh scripts/linux1/mk_disk.sh
git mv scripts/run_linux1_qemu.sh scripts/linux1/run_qemu.sh
git mv scripts/run_linux1_curses.sh scripts/linux1/run_curses.sh
git mv scripts/verify_linux1_provenance.sh scripts/linux1/verify_provenance.sh
git mv scripts/linux1_userspace.ld scripts/linux1/userspace.ld
git mv scripts/linux1_zsystem.ld scripts/linux1/zsystem.ld
git mv scripts/build_xv6.sh scripts/xv6/build.sh
```

- [ ] **Step 2: Update internal script path resolution**

Update Linux1 scripts so `ROOT` resolves from two levels up:

```bash
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
```

Update Linux1 path references:

```bash
MANIFEST="$ROOT/linux-1.0.0/manifests/linux1-sources.lock"
SRC="$ROOT/linux-1.0.0/userspace/src"
LINKER_SCRIPT="$ROOT/scripts/linux1/userspace.ld"
```

Patch references must become:

```bash
"$ROOT/linux-1.0.0/patches/kernel/*.patch"
"$ROOT/linux-1.0.0/patches/lilo/*.patch"
```

- [ ] **Step 3: Commit the script regrouping**

```bash
git add scripts/linux1 scripts/xv6
git commit -m "refactor: group linux1 and xv6 scripts by owner"
```

### Task 5: Repoint Makefile And Any Remaining Runtime References

**Files:**
- Modify: `Makefile`
- Modify: `tests/test_linux1_make_targets.py`
- Modify: `tests/test_xv6.py`
- Modify: `tests/test_linux1.py`
- Modify: `tests/test_linux1_userspace_layout.py`
- Modify: `tests/test_linux1_kernel_layout.py`
- Modify: `tests/test_linux1_disk_geometry.py`

- [ ] **Step 1: Update top-level recipes to use grouped script paths**

Replace these `Makefile` lines:

```make
@bash scripts/build_xv6.sh
@bash scripts/fetch_linux1_sources.sh
@bash scripts/build_linux1_kernel.sh
@bash scripts/build_linux1_userspace.sh
@bash scripts/build_linux1_lilo.sh
@bash scripts/mk_linux1_disk.sh
@bash scripts/run_linux1_qemu.sh
@bash scripts/run_linux1_curses.sh
```

with:

```make
@bash scripts/xv6/build.sh
@bash scripts/linux1/fetch_sources.sh
@bash scripts/linux1/build_kernel.sh
@bash scripts/linux1/build_userspace.sh
@bash scripts/linux1/build_lilo.sh
@bash scripts/linux1/mk_disk.sh
@bash scripts/linux1/run_qemu.sh
@bash scripts/linux1/run_curses.sh
```

- [ ] **Step 2: Run the targeted layout and path tests**

Run:

```bash
pytest \
  tests/test_repo_layout.py \
  tests/test_linux1_make_targets.py \
  tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py \
  tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py \
  tests/test_xv6.py -v
```

Expected: PASS.

- [ ] **Step 3: Commit the path repointing**

```bash
git add Makefile tests/test_repo_layout.py tests/test_linux1_make_targets.py tests/test_linux1.py tests/test_linux1_userspace_layout.py tests/test_linux1_kernel_layout.py tests/test_linux1_disk_geometry.py tests/test_xv6.py
git commit -m "build: repoint repo entrypaths to ownership layout"
```

## Chunk 3: Update Documentation And Run Final Verification

### Task 6: Update Repo Documentation To Match The New Ownership Layout

**Files:**
- Modify: `README.md`
- Modify: `docs/superpowers/specs/2026-03-13-linux1-authentic-build-design.md`
- Modify: `docs/superpowers/plans/2026-03-13-linux1-authentic-build.md`
- Modify: `docs/superpowers/specs/2026-03-13-xv6-port-design.md`
- Modify: `docs/superpowers/plans/2026-03-13-xv6-port.md`

- [ ] **Step 1: Update current operator docs first**

In `README.md`, change the Linux1 workflow list from:

```text
scripts/fetch_linux1_sources.sh
scripts/build_linux1_kernel.sh
scripts/build_linux1_userspace.sh
scripts/build_linux1_lilo.sh
scripts/mk_linux1_disk.sh
scripts/run_linux1_qemu.sh
```

to:

```text
scripts/linux1/fetch_sources.sh
scripts/linux1/build_kernel.sh
scripts/linux1/build_userspace.sh
scripts/linux1/build_lilo.sh
scripts/linux1/mk_disk.sh
scripts/linux1/run_qemu.sh
```

Also mention Linux1 userspace and source manifest now live under `linux-1.0.0/`.

- [ ] **Step 2: Update historical design and plan docs where they point at the old live paths**

Only change literal repository paths that would now mislead a reader. Do not rewrite the historical narrative beyond path correction.

- [ ] **Step 3: Commit the documentation updates**

```bash
git add README.md docs/superpowers/specs/2026-03-13-linux1-authentic-build-design.md docs/superpowers/plans/2026-03-13-linux1-authentic-build.md docs/superpowers/specs/2026-03-13-xv6-port-design.md docs/superpowers/plans/2026-03-13-xv6-port.md
git commit -m "docs: update paths for repo ownership reorg"
```

### Task 7: Run The Final Verification Slice And Finish Cleanly

**Files:**
- Modify: none
- Verify: `Makefile`
- Verify: `README.md`
- Verify: `scripts/linux1/*`
- Verify: `scripts/xv6/build.sh`
- Verify: `tests/test_repo_layout.py`
- Verify: `tests/test_linux1*.py`
- Verify: `tests/test_xv6.py`

- [ ] **Step 1: Run the final repo-ownership verification commands**

Run:

```bash
pytest \
  tests/test_repo_layout.py \
  tests/test_linux1_make_targets.py \
  tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py \
  tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py \
  tests/test_xv6.py -v
```

Expected: PASS.

Run:

```bash
pytest tests/test_linux_lab_phase1_repo.py tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_phase1_plan.py -v
```

Expected: PASS. This confirms the reorg did not disturb the already-landed `linux-lab` baseline.

- [ ] **Step 2: Run the top-level entrypoint smoke checks**

Run:

```bash
make -n linux1
make -n linux1-curses
make -n xv6
```

Expected: the printed recipes call `scripts/linux1/...` and `scripts/xv6/build.sh`, not the removed flat script names.

- [ ] **Step 3: Check git state and commit any last verification-only fixups**

Run:

```bash
git status --short
```

Expected: clean working tree.

If a final fixup was needed during verification:

```bash
git add <exact files>
git commit -m "fix: close repo ownership reorg gaps"
```
