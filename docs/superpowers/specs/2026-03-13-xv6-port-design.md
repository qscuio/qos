# xv6 Port Design Specification

**Date:** 2026-03-13
**Status:** Approved for implementation

## Overview

Port xv6-x86 (MIT's original i386 teaching OS) into the QOS monorepo as a self-contained subdirectory alongside `c-os/` and `rust-os/`. Add a build script and a pytest test suite covering build verification and QEMU boot smoke testing.

## Goals

- Give students a reference bootable OS to compare against the QOS implementations
- Integrate cleanly with the existing monorepo build and test conventions
- Run on the AArch64 host via cross-compilation and QEMU i386 emulation

## Non-Goals

- Modifying xv6 source beyond what is necessary to build on an AArch64 host
- Integrating xv6 with the QOS kernel, libc, or userspace
- Adding new xv6 features or porting to x86_64/AArch64

## Directory Layout

```
qos/
├── xv6/
│   ├── Makefile          # upstream xv6 Makefile, patched for cross-compilation
│   ├── kernel/           # kernel C sources and headers
│   ├── user/             # user programs (sh, ls, cat, echo, etc.)
│   ├── boot/             # bootasm.S, bootmain.c
│   ├── fs/               # mkfs and filesystem image tools
│   └── build/            # compiled objects, xv6.img (gitignored)
├── scripts/
│   └── build_xv6.sh      # thin wrapper: invokes make with cross-compiler flags
├── tests/
│   └── test_xv6.py       # pytest: build check + boot smoke test
└── Makefile              # adds `xv6` and `test-xv6` targets
```

The `xv6/build/` directory is gitignored, matching the convention of `c-os/build/` and `rust-os/build/`.

## Source

Copy xv6-x86 source verbatim from the MIT upstream repository:
`https://github.com/mit-pdos/xv6-public`

Apply only the minimum patches required to build on an AArch64 host with the i686 cross-compiler. No functional changes to xv6.

## Build System

### Host Prerequisites

```
apt install gcc-multilib gcc-i686-linux-gnu qemu-system-misc
```

`qemu-system-misc` provides `qemu-system-i386` on Ubuntu AArch64.

### Cross-Compilation

xv6-x86 targets i386. On an AArch64 host, build using the i686 cross-compiler:

- `TOOLPREFIX=i686-linux-gnu-`
- Assembler: `i686-linux-gnu-as`
- Linker: `i686-linux-gnu-ld`
- Objcopy: `i686-linux-gnu-objcopy`

### `scripts/build_xv6.sh`

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../xv6"
make TOOLPREFIX=i686-linux-gnu- xv6.img
```

### `xv6/Makefile` Patches

- Set default `TOOLPREFIX=i686-linux-gnu-`
- Redirect build artifacts to `xv6/build/`

### Root `Makefile` Additions

```makefile
xv6:
    @bash scripts/build_xv6.sh

test-xv6:
    @pytest tests/test_xv6.py -v
```

The existing `test-all` target is extended to include `xv6` build and `test-xv6`.

## Test Suite (`tests/test_xv6.py`)

Two pytest test cases following the style of existing `test_qemu_*.py` tests.

### Test 1: Build Check

Invokes `scripts/build_xv6.sh` via `subprocess.run`. Asserts:
- Exit code is 0
- `xv6/build/xv6.img` exists after the build

### Test 2: Boot Smoke Test

Launches `qemu-system-i386` with:
- `-nographic` (serial output to stdout, no display window)
- `-drive file=xv6/build/xv6.img,format=raw`

Reads stdout until the xv6 shell prompt (`$`) appears or a 30-second timeout expires. Asserts the prompt is found, then terminates the QEMU process.

```python
import subprocess, os, time
from pathlib import Path
from subprocess import PIPE, STDOUT

REPO_ROOT = Path(__file__).parent.parent

def test_xv6_build():
    result = subprocess.run(
        ["bash", "scripts/build_xv6.sh"],
        cwd=REPO_ROOT,
        capture_output=True,
    )
    assert result.returncode == 0, result.stderr.decode()
    assert (REPO_ROOT / "xv6/build/xv6.img").exists()

def test_xv6_boot_smoke():
    proc = subprocess.Popen(
        [
            "qemu-system-i386", "-nographic",
            "-drive", f"file={REPO_ROOT}/xv6/build/xv6.img,format=raw",
        ],
        stdout=PIPE,
        stderr=STDOUT,
    )
    try:
        deadline = time.time() + 30
        buf = b""
        while time.time() < deadline:
            chunk = proc.stdout.read(256)
            if not chunk:
                break
            buf += chunk
            if b"$ " in buf:
                return  # success
        assert False, f"xv6 shell prompt not found within 30s. Output:\n{buf.decode(errors='replace')}"
    finally:
        proc.kill()
        proc.wait()
```

## Integration with Existing Tests

`test_xv6.py` is a standalone pytest file. It does not share fixtures with the existing kernel/QEMU tests (which target x86_64/AArch64 64-bit guests). This avoids conflicts with the existing `conftest.py` harness.

## Prerequisites Documentation

The `xv6/README` and the top-level `README.md` are updated to document the required `apt` packages and build commands.
