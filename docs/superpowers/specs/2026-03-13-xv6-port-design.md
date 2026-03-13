# xv6 Port Design Specification

**Date:** 2026-03-13
**Status:** Approved for implementation

## Overview

Port xv6-x86 (MIT's original i386 teaching OS) into the QOS monorepo as a self-contained
subdirectory alongside `c-os/` and `rust-os/`. Add a build script and a pytest test suite
covering build verification and QEMU boot smoke testing.

## Goals

- Give students a reference bootable OS to compare against the QOS implementations
- Integrate cleanly with the existing monorepo build and test conventions
- Run on the AArch64 host via cross-compilation and QEMU i386 emulation

## Non-Goals

- Modifying xv6 source beyond what is necessary to build on an AArch64 host
- Integrating xv6 with the QOS kernel, libc, or userspace
- Adding new xv6 features or porting to x86_64/AArch64

---

## Source Import

Copy the xv6-x86 source verbatim from the MIT upstream repository:
`https://github.com/mit-pdos/xv6-public`

The source is vendored as committed files in `xv6/` — it is **not** a git submodule. The
one-time import command:

```bash
git clone --depth=1 https://github.com/mit-pdos/xv6-public /tmp/xv6-public
cp -r /tmp/xv6-public/. xv6/
rm -rf xv6/.git
```

Apply only the minimum patches required to build on an AArch64 host. No functional changes
to xv6 are permitted.

---

## Directory Layout

The upstream `mit-pdos/xv6-public` repository stores all sources flat in the root (no
subdirectories for `kernel/`, `user/`, etc.). The layout in this repo matches that structure:

```
qos/
├── xv6/                      # all xv6 sources, flat as in upstream
│   ├── Makefile              # upstream Makefile, patched (see below)
│   ├── proc.c, vm.c, ...     # kernel sources
│   ├── sh.c, ls.c, ...       # user program sources
│   ├── bootasm.S, bootmain.c # bootloader
│   ├── mkfs.c                # filesystem image builder
│   ├── .gitignore            # ignores build artifacts
│   └── README                # updated with prerequisites and build instructions
├── scripts/
│   └── build_xv6.sh          # thin wrapper: invokes make with cross-compiler flags
├── tests/
│   └── test_xv6.py           # pytest: build check + boot smoke test
└── Makefile                  # xv6/xv6-clean/test-xv6 targets added; test-all/clean extended
```

Build artifacts (`.o`, `.d`, `xv6.img`, `fs.img`, `bootblock`, `kernel`) are gitignored via
`xv6/.gitignore`. There is no `xv6/build/` subdirectory — artifacts remain in `xv6/` root,
matching upstream behaviour.

---

## Host Prerequisites

```
apt install gcc-i686-linux-gnu binutils-i686-linux-gnu qemu-system-x86
```

Notes:
- **`gcc-multilib` must NOT be listed** — it is not installable on AArch64.
- `qemu-system-x86` provides `qemu-system-i386` (confirmed present at
  `/usr/bin/qemu-system-i386` on this host via the `qemu-system-x86` package).
- `gcc-i686-linux-gnu` provides the `i686-linux-gnu-gcc` cross-compiler.

---

## Build System

### `xv6/Makefile` Patches

No Makefile patches are required. The upstream Makefile uses a `ifndef TOOLPREFIX` guard,
so passing `TOOLPREFIX=i686-linux-gnu-` on the command line in `build_xv6.sh` overrides the
auto-detection entirely. The `QEMU` variable defaults to `qemu-system-i386`, which is correct.

The build script builds both `xv6.img` and `fs.img` — both are required for a bootable xv6.

### `scripts/build_xv6.sh`

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT/xv6"
make TOOLPREFIX=i686-linux-gnu- xv6.img fs.img
```

Both `xv6.img` (boot + kernel) and `fs.img` (root filesystem) are required. Without `fs.img`,
xv6 panics in `ide.c` before the shell prompt is printed.

`ROOT` resolution uses `${BASH_SOURCE[0]}`, matching the convention in other `scripts/` files,
so the script works correctly regardless of the working directory from which it is invoked.

### Root `Makefile` Changes

The existing `test-all` and `clean` targets are **edited in-place** (not duplicated). Duplicate
targets in GNU Make silently drop the earlier recipe. New targets `xv6`, `xv6-clean`, and
`test-xv6` are added. Parallel `make -j` is not supported — `test-xv6` has no declared
dependency on `xv6` in the Makefile; the build is handled by the pytest fixture instead.

**`test-all` prerequisite:** `gcc-i686-linux-gnu` and `qemu-system-x86` must be installed for
`test-all` to pass. The existing `test_qemu_serial_smoke.py::test_test_all_uses_real_qemu_serial_markers`
test runs `make test-all` — it will only pass once these packages are installed. Both packages
are available on this host and must be declared as CI prerequisites.

**Diff against the existing root `Makefile`:**

```diff
--- a/Makefile
+++ b/Makefile
@@ -1,6 +1,6 @@
 ARCHES := x86_64 aarch64
 ARCH_GOAL := $(firstword $(filter $(ARCHES),$(MAKECMDGOALS)))

-.PHONY: c rust test-all clean $(ARCHES)
+.PHONY: c rust test-all xv6 xv6-clean test-xv6 clean $(ARCHES)

 c:
 	@if [ -z "$(ARCH_GOAL)" ]; then \
@@ -16,10 +16,16 @@ rust:
 	@$(MAKE) -C rust-os ARCH=$(ARCH_GOAL) build

 test-all:
 	@$(MAKE) -C c-os ARCH=x86_64 smoke
 	@$(MAKE) -C c-os ARCH=aarch64 smoke
 	@$(MAKE) -C rust-os ARCH=x86_64 smoke
 	@$(MAKE) -C rust-os ARCH=aarch64 smoke
+	@bash scripts/build_xv6.sh
+	@pytest tests/test_xv6.py -v

+xv6:
+	@bash scripts/build_xv6.sh
+
+xv6-clean:
+	@$(MAKE) -C xv6 clean
+
+test-xv6:
+	@pytest tests/test_xv6.py -v
+
 clean:
 	@$(MAKE) -C c-os clean
 	@$(MAKE) -C rust-os clean
+	@$(MAKE) -C xv6 clean

 $(ARCHES):
 	@:
```

---

## Test Suite (`tests/test_xv6.py`)

Two pytest test cases following the style of existing `test_qemu_*.py` tests.

### `ROOT` convention

Matches the existing pattern used throughout `tests/`:

```python
ROOT = Path(__file__).resolve().parents[1]
```

### Module-level build fixture

A module-scoped `autouse` fixture builds `xv6.img` before any test in this file runs.
This means `pytest tests/test_xv6.py` works standalone without a prior `make xv6`.

The fixture combines stdout and stderr in the failure message so all compiler errors are
visible. A 5-minute timeout guards against a hung build.

### Boot smoke test I/O

`proc.communicate(timeout=30)` is used for QEMU output collection. This is simpler and
correct for a smoke test: QEMU produces a burst of output while booting, then stalls at the
shell prompt. `communicate()` will block until QEMU exits (it won't for a live process), but
the `timeout=30` causes it to raise `subprocess.TimeoutExpired` which is caught and inspected.
At that point the buffered output is checked for the shell prompt — a timeout during
`communicate()` on a live QEMU process is the *expected* success path (QEMU is still running
at the prompt).

### Complete `tests/test_xv6.py`

```python
import subprocess
import time
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
XV6_IMG = ROOT / "xv6" / "xv6.img"
XV6_FS_IMG = ROOT / "xv6" / "fs.img"


@pytest.fixture(scope="module", autouse=True)
def build_xv6():
    result = subprocess.run(
        ["bash", "scripts/build_xv6.sh"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=300,
    )
    assert result.returncode == 0, (
        f"xv6 build failed:\n{result.stdout.decode(errors='replace')}"
    )


def test_xv6_build():
    assert XV6_IMG.exists(), f"xv6.img not found at {XV6_IMG}"
    assert XV6_FS_IMG.exists(), f"fs.img not found at {XV6_FS_IMG}"


def test_xv6_boot_smoke():
    proc = subprocess.Popen(
        [
            "qemu-system-i386",
            "-nographic",
            "-serial", "stdio",
            "-drive", f"file={XV6_IMG},index=0,media=disk,format=raw",
            "-drive", f"file={XV6_FS_IMG},index=1,media=disk,format=raw",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    try:
        out, _ = proc.communicate(timeout=30)
    except subprocess.TimeoutExpired:
        # Expected: QEMU is still running at the shell prompt.
        # Kill it and read whatever was buffered.
        proc.kill()
        out, _ = proc.communicate()

    text = out.decode(errors="replace")
    assert "$ " in text, (
        f"xv6 shell prompt ('$ ') not found in QEMU output.\n"
        f"Output:\n{text}"
    )
```

**Why `-serial stdio`:** xv6's kernel writes boot messages and the shell prompt to the serial
port (COM1). `-nographic` alone routes VGA and serial to stdio on modern QEMU, but explicitly
passing `-serial stdio` is consistent with xv6's own `qemu-nox` Makefile target and removes
ambiguity.

---

## `xv6/.gitignore`

```
# Build artifacts (generated by make)
*.o
*.d
*.asm       # disassembly output, not source files
*.sym       # symbol table output, not source files
bootblock
entryother
initcode
kernel
xv6.img
fs.img
mkfs
```

---

## README Updates

`xv6/README` gains a section at the top (before the existing content):

```
== Building in the QOS monorepo ==

Prerequisites (AArch64 Ubuntu host):
  apt install gcc-i686-linux-gnu binutils-i686-linux-gnu qemu-system-x86

Build:
  make xv6            # from repo root — builds xv6.img
  make test-xv6       # build + boot smoke test
  make xv6-clean      # remove build artifacts
```

The top-level `README.md` gains an `xv6` row in whatever project overview section lists
`c-os` and `rust-os`, with the description: "xv6-x86 (MIT teaching OS, i386, reference
bootable OS)".
