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
│   ├── .gitignore            # ignores *.o, *.d, xv6.img, fs.img, etc.
│   └── README                # updated with prerequisites and build instructions
├── scripts/
│   └── build_xv6.sh          # thin wrapper: invokes make with cross-compiler flags
├── tests/
│   └── test_xv6.py           # pytest: build check + boot smoke test
└── Makefile                  # adds xv6, test-xv6, xv6-clean targets; extends test-all
```

Build artifacts (`.o`, `.d`, `xv6.img`, `fs.img`, `bootblock`, `kernel`) are gitignored via
`xv6/.gitignore`. There is no `xv6/build/` subdirectory — artifacts remain in `xv6/` root,
matching upstream behaviour and avoiding non-trivial Makefile restructuring.

---

## Host Prerequisites

```
apt install gcc-i686-linux-gnu binutils-i686-linux-gnu qemu-system-x86
```

Notes:
- **`gcc-multilib` must NOT be listed** — it is not installable on AArch64.
- `qemu-system-x86` provides `qemu-system-i386` (confirmed present at `/usr/bin/qemu-system-i386`
  on this host).
- `gcc-i686-linux-gnu` provides the `i686-linux-gnu-gcc` cross-compiler.

---

## Build System

### `xv6/Makefile` Patches

Two minimal patches to the upstream Makefile:

1. Set the default cross-compiler prefix:
   ```makefile
   TOOLPREFIX = i686-linux-gnu-
   ```

2. Ensure `QEMU` points to the correct binary (upstream defaults to `qemu-system-i386`
   which is correct; no change needed if already set).

No other changes. The Makefile target used is `xv6.img` (upstream default).

### `scripts/build_xv6.sh`

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT/xv6"
make TOOLPREFIX=i686-linux-gnu- xv6.img
```

The `ROOT` resolution uses `${BASH_SOURCE[0]}` (matching the convention in other scripts in
`scripts/`), so the script works correctly regardless of the working directory from which it
is invoked.

### Root `Makefile` Additions

```makefile
.PHONY: xv6 xv6-clean test-xv6

xv6:
	@bash scripts/build_xv6.sh

xv6-clean:
	@$(MAKE) -C xv6 clean

test-xv6:
	@pytest tests/test_xv6.py -v

test-all:
	@$(MAKE) -C c-os ARCH=x86_64 smoke
	@$(MAKE) -C c-os ARCH=aarch64 smoke
	@$(MAKE) -C rust-os ARCH=x86_64 smoke
	@$(MAKE) -C rust-os ARCH=aarch64 smoke
	@bash scripts/build_xv6.sh
	@pytest tests/test_xv6.py -v

clean:
	@$(MAKE) -C c-os clean
	@$(MAKE) -C rust-os clean
	@$(MAKE) -C xv6 clean
```

---

## Test Suite (`tests/test_xv6.py`)

Two pytest test cases following the style of existing `test_qemu_*.py` tests.

### Module-level build fixture

Both tests require `xv6.img` to exist. A module-scoped `autouse` fixture ensures the image is
built before any test runs:

```python
@pytest.fixture(scope="module", autouse=True)
def build_xv6(tmp_path_factory):
    result = subprocess.run(
        ["bash", "scripts/build_xv6.sh"],
        cwd=REPO_ROOT,
        capture_output=True,
    )
    assert result.returncode == 0, result.stderr.decode()
```

This means `pytest tests/test_xv6.py` always works standalone — no prior `make xv6` required.

### Test 1: Build Check

Verifies the image was produced by the fixture:

```python
def test_xv6_build():
    assert (REPO_ROOT / "xv6" / "xv6.img").exists()
```

### Test 2: Boot Smoke Test

Launches `qemu-system-i386` and reads serial output until the xv6 shell prompt (`$ `) appears
or a 30-second timeout expires.

**I/O handling:** Uses `proc.stdout.read1(256)` (available on `BufferedReader`) which returns
as soon as any bytes are available without blocking until the buffer fills. This avoids the
blocking `read(256)` pitfall on a live process.

```python
import subprocess, time
from pathlib import Path
from subprocess import PIPE, STDOUT
import pytest

REPO_ROOT = Path(__file__).parent.parent

@pytest.fixture(scope="module", autouse=True)
def build_xv6():
    result = subprocess.run(
        ["bash", "scripts/build_xv6.sh"],
        cwd=REPO_ROOT,
        capture_output=True,
    )
    assert result.returncode == 0, result.stderr.decode()

def test_xv6_build():
    assert (REPO_ROOT / "xv6" / "xv6.img").exists()

def test_xv6_boot_smoke():
    img = REPO_ROOT / "xv6" / "xv6.img"
    proc = subprocess.Popen(
        [
            "qemu-system-i386",
            "-nographic",
            "-drive", f"file={img},index=0,media=disk,format=raw",
        ],
        stdout=PIPE,
        stderr=STDOUT,
    )
    try:
        deadline = time.time() + 30
        buf = b""
        while time.time() < deadline:
            # read1 returns as soon as any bytes are available (non-blocking wrt buffer)
            chunk = proc.stdout.read1(256)
            if chunk:
                buf += chunk
                if b"$ " in buf:
                    return  # success
            elif proc.poll() is not None:
                # QEMU exited before prompt
                break
        rc = proc.poll()
        if rc is not None and rc != 0:
            pytest.fail(
                f"qemu-system-i386 exited with code {rc} before shell prompt.\n"
                f"Output:\n{buf.decode(errors='replace')}"
            )
        else:
            pytest.fail(
                f"xv6 shell prompt ('$ ') not found within 30s.\n"
                f"Output:\n{buf.decode(errors='replace')}"
            )
    finally:
        proc.kill()
        proc.wait()
```

---

## `xv6/.gitignore`

```
*.o
*.d
*.asm
*.sym
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

`xv6/README` gains a section at the top:

```
== Building in the QOS monorepo ==

Prerequisites (AArch64 Ubuntu host):
  apt install gcc-i686-linux-gnu binutils-i686-linux-gnu qemu-system-x86

Build:
  make xv6            # from repo root
  make test-xv6       # build + boot smoke test
  make xv6-clean      # remove artifacts
```

The top-level `README.md` gains a brief `xv6` entry in the project overview table.
