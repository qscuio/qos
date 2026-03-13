# Linux 1.0.0 Authentic Build-and-Boot Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Integrate Linux 1.0.0 as an in-repo learning kernel with a reproducible historical boot chain (BIOS -> MBR -> LILO -> kernel -> init -> shell prompt) and automated smoke tests.

**Architecture:** Keep `linux-1.0.0/` committed in-repo. Fetch all external historical sources via pinned manifest + SHA256 checks. Build syscall-only i386 static userspace in-repo. Assemble ext2 disk image with LILO installed to MBR, boot via QEMU disk path, and assert ordered boot markers from serial logs.

**Tech Stack:** Linux 1.0.0 source (historic kernel), LILO source build, bash scripts, GNU make, `qemu-system-i386`, `gcc-i686-linux-gnu`, `binutils-i686-linux-gnu`, `e2fsprogs`, pytest.

---

## Chunk 1: Baseline Files, Manifest, and RED Tests

### Task 1: Add Linux1 source/ignore scaffolding

**Files:**
- Create: `linux1-userspace/README.md`
- Create: `linux1-userspace/src/init.S`
- Create: `linux1-userspace/src/sh.S`
- Create: `linux1-userspace/src/echo.S`
- Create: `manifests/linux1-sources.lock`
- Modify: `.gitignore`

- [ ] **Step 1: Create pinned source manifest**

Create `manifests/linux1-sources.lock` with exact fields per line:

```text
# name|url|sha256|filename
linux-1.0.0|https://cdn.kernel.org/pub/linux/kernel/Historic/linux-1.0.tar.gz|AUTO_SHA256|linux-1.0.tar.gz
lilo-20|https://www.ibiblio.org/pub/Linux/system/boot/lilo/lilo-20.tar.gz|AUTO_SHA256|lilo-20.tar.gz
```

Note: `AUTO_SHA256` is a deliberate bootstrap marker. It is replaced with pinned hashes in Chunk 2, Task 3, Step 2.

- [ ] **Step 2: Add linux1 artifact ignores**

Append to `.gitignore`:

```gitignore
# linux1 generated artifacts
build/linux1/
third_party/linux1/
```

- [ ] **Step 3: Create `linux1-userspace/README.md`**

Create a short file documenting that this tree contains syscall-only i386 userspace sources for Linux 1.0.0 smoke boot.

- [ ] **Step 4: Create `linux1-userspace/src/init.S` placeholder**

Create an assembly placeholder that intentionally fails final behavior tests until implementation is added:

```asm
/* linux1-userspace/src/init.S */
.globl _start
_start:
  mov $1, %eax
  mov $1, %ebx
  int $0x80
```

- [ ] **Step 5: Create `linux1-userspace/src/sh.S` placeholder**

Create a matching minimal `_start` exit stub for `sh.S`.

- [ ] **Step 6: Create `linux1-userspace/src/echo.S` placeholder**

Create a matching minimal `_start` exit stub for `echo.S`.

- [ ] **Step 7: Verify scaffold file existence and manifest format**

```bash
test -f manifests/linux1-sources.lock
test -f linux1-userspace/README.md
test -f linux1-userspace/src/init.S
test -f linux1-userspace/src/sh.S
test -f linux1-userspace/src/echo.S
awk -F'|' 'NF==4 || /^#/ {next} {bad=1} END {exit bad}' manifests/linux1-sources.lock
echo \"OK: linux1 scaffolding baseline\"
```

Expected: final output includes `OK: linux1 scaffolding baseline`.

- [ ] **Step 8: Commit scaffolding**

```bash
git add manifests/linux1-sources.lock .gitignore linux1-userspace/
git commit -m "chore: add linux1 source manifest and userspace scaffolding"
```

---

### Task 2: Write failing Linux1 tests first

**Files:**
- Create: `tests/test_linux1.py`

- [ ] **Step 1: Create failing pytest module**

```python
import os
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
LOG = ROOT / "build" / "linux1" / "logs" / "boot.log"
DISK = ROOT / "build" / "linux1" / "images" / "linux1-disk.img"
KERNEL = ROOT / "build" / "linux1" / "kernel" / "vmlinuz-1.0.0"


@pytest.fixture(scope="module", autouse=True)
def build_linux1():
    timeout_sec = int(os.environ.get("LINUX1_BUILD_TIMEOUT_SEC", "1800"))
    result = subprocess.run(
        ["make", "linux1"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert result.returncode == 0, result.stdout.decode(errors="replace")


def test_linux1_kernel_build():
    assert KERNEL.exists(), f"missing kernel image: {KERNEL}"


def test_linux1_disk_boot_lilo():
    assert DISK.exists(), f"missing boot disk: {DISK}"
    text = LOG.read_text(errors="replace") if LOG.exists() else ""
    assert "LILO" in text, f"LILO marker not found in {LOG}\n{text}"


def test_linux1_boot_to_init_shell():
    text = LOG.read_text(errors="replace") if LOG.exists() else ""
    ordered = ["LILO", "Linux version 1.0.0", "linux1-init: start", "linux1-sh# "]
    pos = -1
    for marker in ordered:
        nxt = text.find(marker, pos + 1)
        assert nxt >= 0, f"marker missing: {marker}\n{text}"
        pos = nxt
```

- [ ] **Step 2: Run tests and verify RED**

```bash
pytest tests/test_linux1.py -v
```

Expected: fixture fails because `make linux1` target/scripts do not exist yet.

- [ ] **Step 3: Commit failing tests**

```bash
git add tests/test_linux1.py
git commit -m "test: add failing linux1 build and boot smoke tests"
```

---

## Chunk 2: Implement Fetch/Build/Boot Pipeline

### Task 3: Implement source fetch + checksum + provenance scripts

**Files:**
- Create: `scripts/fetch_linux1_sources.sh`
- Create: `scripts/verify_linux1_provenance.sh`

- [ ] **Step 1: Implement fetch script**

Script requirements:
- `set -euo pipefail`
- parse `manifests/linux1-sources.lock`
- download into `build/linux1/sources/`
- if hash in manifest is `AUTO_SHA256`, compute and print it, then fail with exit 2
- if hash is set, enforce SHA256 match
- emit failure lines in contract form: `ERROR:fetch-linux1-sources:<reason>` with exit 1/2 contract compliance

- [ ] **Step 2: Fill manifest SHA256 values once and rerun fetch**

```bash
bash scripts/fetch_linux1_sources.sh || true
# update manifest with printed SHA256 values
bash scripts/fetch_linux1_sources.sh
```

Expected: exit 0 with verified archives present.

- [ ] **Step 3: Implement provenance verifier**

`verify_linux1_provenance.sh` must:
- unpack kernel tarball to temp dir
- apply `patches/linux-1.0.0/*.patch` (if any)
- compare resulting tree to committed `linux-1.0.0/` (excluding generated files)
- verify LILO source extraction + `patches/lilo/*.patch` applies cleanly
- verify LILO build artifact contract (`build/linux1/lilo/` exists, contains built `lilo` binary, and reports expected version signature)
- print `OK: provenance` on success

- [ ] **Step 4: Verify fetch script behavior before commit**

```bash
bash scripts/fetch_linux1_sources.sh
```

Expected: exit 0 with verified sources and no checksum mismatches.

- [ ] **Step 5: Commit scripts**

```bash
git add scripts/fetch_linux1_sources.sh scripts/verify_linux1_provenance.sh manifests/linux1-sources.lock
git commit -m "build: add linux1 fetch and provenance verification scripts"
```

---

### Task 4: Vendor Linux 1.0.0 kernel source in-repo

**Files:**
- Create: `linux-1.0.0/` (vendored source)
- Create: `patches/linux-1.0.0/` (as needed)

- [ ] **Step 1: Extract kernel source from pinned archive**

```bash
mkdir -p /tmp/linux1-import
tar -xf build/linux1/sources/linux-1.0.tar.gz -C /tmp/linux1-import
rm -rf linux-1.0.0
cp -a /tmp/linux1-import/linux/. linux-1.0.0/
```

- [ ] **Step 2: Apply minimal compatibility patches (if build requires)**

```bash
if ls patches/linux-1.0.0/*.patch >/dev/null 2>&1; then
  for p in patches/linux-1.0.0/*.patch; do
    patch -d linux-1.0.0 -p1 < "$p"
  done
fi
```

Expected: all patches apply without rejects.

- [ ] **Step 3: Commit vendored kernel**

```bash
git add linux-1.0.0 patches/linux-1.0.0
git commit -m "chore: vendor linux 1.0.0 kernel source with minimal compatibility patches"
```

---

### Task 5: Implement kernel/LILO/userspace build scripts

**Files:**
- Create: `scripts/build_linux1_kernel.sh`
- Create: `scripts/build_linux1_lilo.sh`
- Create: `scripts/build_linux1_userspace.sh`
- Create: `linux1-userspace/src/syscalls.S`
- Create: `linux1-userspace/src/common.inc`
- Modify: `linux1-userspace/src/init.S`
- Modify: `linux1-userspace/src/sh.S`

- [ ] **Step 1: Implement kernel build script**

Requirements:
- output `build/linux1/kernel/vmlinuz-1.0.0`
- use i386 cross toolchain (`i686-linux-gnu-`)
- emit `ERROR:build-kernel:<reason>` and exit 1/2 per contract

- [ ] **Step 2: Implement LILO build script**

Requirements:
- extract lilo archive from `build/linux1/sources/`
- apply `patches/lilo/*.patch` (if present)
- build artifacts into `build/linux1/lilo/`
- emit standard error contract

- [ ] **Step 3: Implement syscall-only userspace build script**

Requirements:
- build static i386 binaries without glibc runtime dependency
- produce:
  - `build/linux1/rootfs/sbin/init`
  - `build/linux1/rootfs/bin/sh`
  - `build/linux1/rootfs/bin/echo`
- create device nodes in staging tree metadata plan (actual nodes created in disk assembly phase)
- emit failure lines in contract form: `ERROR:build-linux1-userspace:<reason>` with exit 1/2 contract compliance

- [ ] **Step 4: Implement init/shell marker behavior**

`/sbin/init` must print `linux1-init: start`, exec `/bin/sh`, and respawn shell on exit.
`/bin/sh` must print prompt `linux1-sh# `.

- [ ] **Step 5: Verify build scripts in isolation**

```bash
bash scripts/build_linux1_kernel.sh
bash scripts/build_linux1_lilo.sh
bash scripts/build_linux1_userspace.sh
```

Expected: outputs created under `build/linux1/{kernel,lilo,rootfs}`.

- [ ] **Step 6: Commit build scripts/userspace**

```bash
git add scripts/build_linux1_kernel.sh scripts/build_linux1_lilo.sh scripts/build_linux1_userspace.sh linux1-userspace/
git commit -m "build: add linux1 kernel lilo and syscall-only userspace build scripts"
```

---

### Task 6: Implement disk assembly and QEMU run scripts

**Files:**
- Create: `scripts/mk_linux1_disk.sh`
- Create: `scripts/run_linux1_qemu.sh`

- [ ] **Step 1: Implement deterministic ext2 disk assembly**

Requirements:
- create `build/linux1/images/linux1-disk.img`
- deterministic partition layout
- ext2 options compatible with Linux 1.0.0:

```bash
mke2fs -t ext2 -O none -I 128 -b 1024
```

- write `lilo.conf` enabling serial and `console=ttyS0`
- install LILO into MBR using built LILO artifacts
- create `/dev/console` and `/dev/null` in rootfs
- verify ext2 features after creation (for example `dumpe2fs -h <fs-dev>` check no unsupported feature flags) and fail if incompatible
- detect privilege requirement early; if insufficient privileges, print one actionable rerun path (for example `sudo bash scripts/mk_linux1_disk.sh`) and exit 2
- emit failure lines in contract form: `ERROR:mk-disk:<reason>` with exit 1/2 contract compliance

- [ ] **Step 2: Implement QEMU runner with cleanup ownership**

Requirements:
- run `qemu-system-i386 -nographic -serial stdio -hda build/linux1/images/linux1-disk.img`
- capture merged output to `build/linux1/logs/boot.log`
- enforce timeout via `LINUX1_BOOT_TIMEOUT_SEC` (default 90)
- always terminate child QEMU before exit
- validate ordered markers (`LILO` -> `Linux version 1.0.0` -> `linux1-init: start` -> `linux1-sh# `) or call a marker validator and propagate non-zero status on marker failure
- emit failure lines in contract form: `ERROR:run-linux1-qemu:<reason>` with exit 1/2 contract compliance

- [ ] **Step 3: Verify marker capture path**

```bash
bash scripts/mk_linux1_disk.sh
bash scripts/run_linux1_qemu.sh
sed -n '1,220p' build/linux1/logs/boot.log
```

Expected markers in order: `LILO`, `Linux version 1.0.0`, `linux1-init: start`, `linux1-sh# `.

- [ ] **Step 4: Commit disk/run scripts**

```bash
git add scripts/mk_linux1_disk.sh scripts/run_linux1_qemu.sh
git commit -m "feat: add linux1 disk assembly and qemu serial runner"
```

---

### Task 7: Integrate root Makefile and docs

**Files:**
- Modify: `Makefile`
- Modify: `README.md`
- Create: `tests/test_linux1_provenance.py`

- [ ] **Step 1: Add Make targets**

Add `.PHONY` entries and targets:
- `linux1`
- `test-linux1`
- `linux1-clean`

`linux1` should call, in order:
- `bash scripts/fetch_linux1_sources.sh`
- `bash scripts/build_linux1_kernel.sh`
- `bash scripts/build_linux1_lilo.sh`
- `bash scripts/build_linux1_userspace.sh`
- `bash scripts/mk_linux1_disk.sh`
- `bash scripts/run_linux1_qemu.sh`

`test-linux1`:
- `pytest tests/test_linux1.py tests/test_linux1_provenance.py -v`

`linux1-clean`:
- `rm -rf build/linux1`

Add provenance CI path:

- create `tests/test_linux1_provenance.py` that runs `bash scripts/verify_linux1_provenance.sh` and asserts exit 0.
- ensure `test-linux1` includes this test so CI that runs project tests enforces provenance.

- [ ] **Step 2: Update README with Linux1 flow**

Document prerequisites, patch policy boundaries, privilege expectation for disk assembly, and commands:

```bash
make linux1
make test-linux1
make linux1-clean
```

Expected: README explicitly states compatibility patches are build-only (no semantic feature changes) and that provenance verification is part of `test-linux1`.

- [ ] **Step 3: Commit integration/docs**

```bash
git add Makefile README.md tests/test_linux1_provenance.py
git commit -m "build: wire linux1 targets and documentation"
```

---

## Chunk 3: Verification and Completion

### Task 8: Green the Linux1 tests and verify contracts

**Files:**
- Modify: `tests/test_linux1.py` (only if needed to align with final log paths/timeouts)
- Modify: `scripts/verify_linux1_provenance.sh` (only if needed to fix final provenance contract gaps)

- [ ] **Step 1: Run Linux1 test module**

```bash
pytest tests/test_linux1.py -v
```

Expected:
- all Linux1 tests pass
- ordered markers validated from `build/linux1/logs/boot.log`

- [ ] **Step 2: Run provenance verification**

```bash
bash scripts/verify_linux1_provenance.sh
```

Expected: `OK: provenance`

- [ ] **Step 3: Run integrated make targets**

```bash
make linux1-clean
make linux1
make test-linux1
```

Expected:
- disk and logs regenerated under `build/linux1/`
- tests pass

- [ ] **Step 4: Commit final test alignment (if any)**

```bash
git add tests/test_linux1.py scripts/verify_linux1_provenance.sh
# only if changes were made in this phase
git commit -m "test: finalize linux1 smoke and provenance verification"
```

---

### Task 9: Final branch validation and handoff

**Files:** none (validation only)

- [ ] **Step 1: Run full targeted verification set**

```bash
pytest -q tests/test_repo_layout.py tests/test_xv6.py tests/test_linux1.py tests/test_linux1_provenance.py
```

Expected: all pass.

- [ ] **Step 2: Summarize artifacts and constraints**

Record in final report:
- generated artifact paths
- privilege expectations for `mk_linux1_disk.sh`
- host support scope (Ubuntu 24.04 x86_64 for milestone 1)

- [ ] **Step 3: Run superpowers:finishing-a-development-branch**

Use branch-finishing workflow to decide merge/PR strategy with user.

Expected: branch-finish decision is recorded (merge, PR, or cleanup path) with rationale.
