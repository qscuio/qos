# Linux 1.0.0 Authentic Build-and-Boot Design

**Date:** 2026-03-13
**Status:** Approved for implementation planning

## Overview

Integrate Linux 1.0.0 into this repository as a historically authentic learning target that can be built and booted end to end. Keep the Linux kernel source local in-repo for study and future development. Build a simple userspace in this repo. Boot through a BIOS disk path with MBR and LILO rather than direct `-kernel` boot.

This design favors historical runtime behavior while allowing minimal build-compatibility patches so the workflow can run on modern Ubuntu hosts.

## User-Confirmed Constraints

- Historical authenticity is required.
- Full disk-boot chain is required: BIOS -> MBR -> LILO -> Linux kernel -> `/sbin/init`.
- Userspace can be simple and built by us in-repo.
- Linux kernel source must stay local in this repo for learning and future work.
- Minimal compatibility patches are allowed.
- External historical sources should be fetched by scripts with pinned URLs and SHA256 checksums, not committed as raw tarballs.

## Goals

- Add Linux 1.0.0 kernel source under version control in this repo.
- Produce a deterministic local workflow to fetch dependencies, build, create bootable disk images, and run in QEMU.
- Preserve a clear patch boundary between original sources and compatibility changes.
- Provide automated smoke tests that prove the authentic boot path and userspace prompt.

## Non-Goals

- Reproduce a perfect 1994 build host byte-for-byte.
- Build an entire historical distribution userland.
- Add modern Linux features into Linux 1.0.0.
- Replace existing `c-os/`, `rust-os/`, or `xv6/` workflows.

## Repository Layout

```text
qos/
├── linux-1.0.0/                         # committed kernel source (local, editable)
├── linux1-userspace/                    # simple in-repo userspace sources
├── patches/
│   ├── linux-1.0.0/                     # compatibility patch set for kernel build
│   └── lilo/                            # compatibility patch set for lilo build/install
├── manifests/
│   └── linux1-sources.lock              # pinned URL + SHA256 for fetched artifacts
├── scripts/
│   ├── fetch_linux1_sources.sh          # fetch pinned external sources + verify SHA256
│   ├── verify_linux1_provenance.sh      # rebuild upstream+patches and compare to committed tree
│   ├── build_linux1_kernel.sh           # build Linux 1.0.0 with compatibility flags/patches
│   ├── build_linux1_lilo.sh             # build lilo from pinned source + patches
│   ├── build_linux1_userspace.sh        # build minimal static userspace
│   ├── mk_linux1_disk.sh                # create partitioned disk, ext2 rootfs, install bootloader
│   └── run_linux1_qemu.sh               # run BIOS disk boot in qemu-system-i386
├── tests/
│   └── test_linux1.py                   # build + authentic boot smoke tests
└── Makefile                             # linux1/test-linux1/linux1-clean integration
```

Generated artifacts are kept under a gitignored output directory (for example `build/linux1/`) so source trees remain clean.

## External Source Strategy

External sources required for authentic boot flow (for example LILO source tarball) are downloaded by script.

- A manifest file in-repo pins:
  - exact URL
  - expected SHA256
  - output filename
- Fetch step fails hard on checksum mismatch.
- Downloaded archives and extracted work directories are gitignored.
- Kernel source remains committed in `linux-1.0.0/`.

Provenance contract:
- `linux-1.0.0/` is the canonical local learning tree.
- `scripts/verify_linux1_provenance.sh` must reproduce a clean upstream extraction, apply the patch stack, and verify tree equivalence to committed `linux-1.0.0/` (ignoring generated files).
- The same provenance verification must cover LILO: pinned source archive, optional `patches/lilo/*.patch`, and deterministic artifact checks for `build_linux1_lilo.sh` outputs.
- The verification command is part of CI for Linux1-related changes.

## Compatibility Patch Policy

Patch scope is strictly limited to portability and build compatibility.

Allowed examples:
- toolchain compatibility for modern GCC/binutils
- Makefile fixes for modern shell utilities
- warning-to-error demotions where needed for unchanged runtime behavior

Not allowed without explicit follow-up approval:
- feature additions
- semantic runtime behavior changes outside compatibility necessity

Patch governance:
- one patch per concern when possible
- each patch includes a short rationale header
- patch application is scripted and deterministic

## Build and Boot Pipeline

1. `fetch`: Download and verify external historical sources.
2. `build-kernel`: Build Linux 1.0.0 i386 kernel from local source with minimal compatibility patches.
3. `build-lilo`: Build LILO from pinned source with minimal compatibility patches.
4. `build-userspace`: Build in-repo minimal static userspace binaries (`/sbin/init`, `/bin/sh`, tiny core tools).
5. `mk-disk`: Create raw disk image, partition it, format ext2 root, install kernel and userspace, install LILO to MBR.
6. `run`: Boot QEMU with `-hda <disk.img>` and serial output capture.

Authentic boot requirement: no direct `qemu -kernel` in the primary smoke path.

Orchestration contract:

- `make linux1` is the single orchestrator and invokes scripts in strict order.
- Each pipeline step maps to exactly one primary script (`fetch` -> `fetch_linux1_sources.sh`, `build-kernel` -> `build_linux1_kernel.sh`, `build-lilo` -> `build_linux1_lilo.sh`, and so on).
- On failure, the orchestrator stops immediately and reports the failing step name.
- Re-run semantics are step-local: rerunning after failure restarts from the failed step with already-produced prerequisite artifacts.

## Script Interfaces

Each script has a strict interface so units are independently testable and composable.

- `fetch_linux1_sources.sh`
  - Inputs: `manifests/linux1-sources.lock`
  - Outputs: verified archives under `build/linux1/sources/`
  - Exit codes: non-zero on download/checksum/manifest parse failure
- `build_linux1_kernel.sh`
  - Inputs: `linux-1.0.0/` and optional `patches/linux-1.0.0/*.patch`
  - Outputs: kernel image under `build/linux1/kernel/`
  - Env: `LINUX1_JOBS` optional parallelism
  - Exit codes: non-zero on patch or build failure
- `build_linux1_lilo.sh`
  - Inputs: pinned LILO source from `build/linux1/sources/` and optional `patches/lilo/*.patch`
  - Outputs: LILO install artifacts under `build/linux1/lilo/`
  - Exit codes: non-zero on patch/configure/build failure
- `build_linux1_userspace.sh`
  - Inputs: `linux1-userspace/`
  - Outputs: staged rootfs files under `build/linux1/rootfs/`
  - Build mode: syscall-only i386 static binaries (`-m32 -nostdlib -static`, no host glibc dependency)
  - Exit codes: non-zero on compile/link/staging failure
- `mk_linux1_disk.sh`
  - Inputs: built kernel + staged rootfs + fetched/built LILO artifacts
  - Outputs: `build/linux1/images/linux1-disk.img`
  - Exit codes: non-zero on partition/fs/bootloader install failure
- `run_linux1_qemu.sh`
  - Inputs: disk image path
  - Outputs: serial log `build/linux1/logs/boot.log`
  - Exit codes: non-zero on launch/timeout/marker failure
  - Cleanup: script owns QEMU process lifecycle and must terminate child QEMU process on timeout or completion before exit

## Artifact Contract

Canonical artifact paths:

- kernel image: `build/linux1/kernel/vmlinuz-1.0.0`
- lilo artifacts: `build/linux1/lilo/`
- rootfs staging dir: `build/linux1/rootfs/`
- bootable disk image: `build/linux1/images/linux1-disk.img`
- boot log: `build/linux1/logs/boot.log`

Scripts must write only to these documented locations (or subpaths) to keep reruns deterministic.

## Runtime Flow and Markers

Expected runtime sequence:

- BIOS executes MBR
- LILO loads Linux kernel from disk
- Linux 1.0.0 kernel boots and mounts root
- `/sbin/init` starts
- userspace shell prompt appears

Smoke assertions check textual markers from each layer:

- bootloader marker (LILO banner or equivalent)
- kernel boot marker
- init/shell prompt marker

Primary marker contract (exact strings):

- bootloader marker: `LILO`
- kernel marker: `Linux version 1.0.0`
- init marker: `linux1-init: start`
- shell prompt marker: `linux1-sh# `

Serial capture contract:

- `mk_linux1_disk.sh` writes a `lilo.conf` that enables serial output and passes kernel console on `ttyS0`.
- `run_linux1_qemu.sh` runs in `-nographic -serial stdio` mode and captures merged output to `build/linux1/logs/boot.log`.
- Tests read `boot.log` and assert markers in order.

## Makefile Integration

Add targets at repository root:

- `linux1`: build kernel, userspace, and bootable disk artifacts
- `test-linux1`: run `pytest tests/test_linux1.py -v`
- `linux1-clean`: remove generated Linux1 artifacts

`test-all` includes Linux1 smoke coverage once Linux1 flow is stable in this repo.

## Testing Strategy

`tests/test_linux1.py` includes at least:

- `test_linux1_kernel_build`
  - runs build script or validates build fixture output
  - asserts kernel image exists
- `test_linux1_disk_boot_lilo`
  - boots disk image via `qemu-system-i386 -hda`
  - asserts `LILO` marker from serial-captured output
- `test_linux1_boot_to_init_shell`
  - asserts `Linux version 1.0.0`, `linux1-init: start`, and `linux1-sh# ` within timeout

Test harness behavior:
- bounded timeout for boot tests
- baseline timeout: 90 seconds, configurable by `LINUX1_BOOT_TIMEOUT_SEC`
- merged stdout/stderr capture
- full captured output included on assertion failure

## Minimal Rootfs and Init Contract

Minimum staged root filesystem contents:

- `/sbin/init` (static i386 binary we build)
- `/bin/sh` (static i386 binary we build)
- `/bin/echo` (static i386 binary we build)
- `/etc/inittab` (if used by init implementation)
- `/dev/console` (char 5:1)
- `/dev/null` (char 1:3)

Userspace ABI contract:

- userspace programs (`/sbin/init`, `/bin/sh`, `/bin/echo`) are built as syscall-only static i386 binaries with local syscall wrappers.
- no dynamic linker and no dependency on host glibc runtime ABI.

`/sbin/init` behavior contract:

- write `linux1-init: start` to console
- ensure stdio is bound to `/dev/console`
- spawn `/bin/sh`
- if shell exits, print `linux1-init: shell exited` and respawn `/bin/sh`
- VM termination is not expected from shell exit; smoke harness controls process lifetime and terminates QEMU after marker capture/timeout

## Error Handling and Determinism

- `set -euo pipefail` in scripts.
- Immediate failure on missing tools, failed downloads, checksum mismatch, patch mismatch, build failure, or boot timeout.
- deterministic disk geometry and partition layout defined in script, not manual commands.
- explicit paths and output dirs so repeated runs are reproducible.

Ext2 compatibility contract:

- `mk_linux1_disk.sh` must create ext2 with options compatible with Linux 1.0.0 (for example `mke2fs -t ext2 -O none -I 128 -b 1024`).
- Script verifies resulting filesystem features and fails if unsupported features are present.

Failure reporting contract:

- Each script logs a step banner at start and an explicit `ERROR:<step>:<reason>` line before non-zero exit.
- Exit code `1` is reserved for runtime/tool failure; exit code `2` is reserved for validation/checksum/contract violations.

## Prerequisites (Host)

Baseline expected packages include:
- `qemu-system-i386`
- `gcc-i686-linux-gnu`
- `binutils-i686-linux-gnu`
- `make`, `bison`, `flex`, `perl`
- ext2 and partition tooling (`e2fsprogs`, `fdisk`/`sfdisk`, loop-mount support)

Supported baseline host (initial milestone): Ubuntu 24.04 x86_64.
AArch64 host support is a follow-up milestone after Linux1 flow is validated on x86_64.

Environment lock guidance:

- CI captures and publishes `dpkg-query` package versions for required toolchain/runtime packages.
- Linux1 verification jobs run against that locked package set to reduce environment drift.

## Security and Safety Notes

- downloaded source verification is mandatory (SHA256)
- scripts should avoid privileged operations where possible
- if loop devices or mount steps require elevated privileges, scripts must fail with explicit instructions

Privilege model:

- `fetch`, `build-kernel`, and `build-userspace` run unprivileged.
- `mk_linux1_disk.sh` may require elevated privileges for loop-device setup, partition writes, and bootloader install.
- `mk_linux1_disk.sh` must detect missing privileges early and emit a single actionable command path (for example rerun with `sudo`).

## Milestone Definition of Done

Linux1 integration is done when all are true:

- `linux-1.0.0/` exists in-repo and is buildable with documented compatibility patches.
- provenance verification passes between committed kernel tree and upstream+patch reconstruction.
- `make linux1` produces a bootable disk image.
- `make test-linux1` passes locally and proves BIOS->MBR->LILO->kernel->init/prompt flow.
- documentation explains fetch/build/run/clean workflow and patch policy.

LILO provenance scope for done criteria:

- source archive checksum verification
- patch stack apply verification
- deterministic command path verification
- artifact presence + expected version/signature checks (source reproducibility, not byte-identical binary reproducibility)

## Open Risks

- Linux 1.0.0 build compatibility on modern host toolchains may require careful patch scoping.
- LILO install behavior can vary with modern host tooling and may need containerization or loop-device constraints.
- boot timings can vary in CI; smoke tests must use robust timeout and marker matching.

## Risk Mitigations

- keep patch set small and traceable
- stage pipeline validation (kernel build first, then disk assembly, then boot)
- capture complete boot logs for fast diagnosis
- keep smoke tests focused on deterministic markers
