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
├── scripts/
│   ├── fetch_linux1_sources.sh          # fetch pinned external sources + verify SHA256
│   ├── build_linux1_kernel.sh           # build Linux 1.0.0 with compatibility flags/patches
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
3. `build-userspace`: Build in-repo minimal static userspace binaries (`/sbin/init`, `/bin/sh`, tiny core tools).
4. `mk-disk`: Create raw disk image, partition it, format ext2 root, install kernel and userspace, install LILO to MBR.
5. `run`: Boot QEMU with `-hda <disk.img>` and serial output capture.

Authentic boot requirement: no direct `qemu -kernel` in the primary smoke path.

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
  - asserts bootloader output marker
- `test_linux1_boot_to_init_shell`
  - asserts kernel marker and prompt marker within timeout

Test harness behavior:
- bounded timeout for boot tests
- merged stdout/stderr capture
- full captured output included on assertion failure

## Error Handling and Determinism

- `set -euo pipefail` in scripts.
- Immediate failure on missing tools, failed downloads, checksum mismatch, patch mismatch, build failure, or boot timeout.
- deterministic disk geometry and partition layout defined in script, not manual commands.
- explicit paths and output dirs so repeated runs are reproducible.

## Prerequisites (Host)

Baseline expected packages include:
- `qemu-system-i386`
- `gcc` / binutils toolchain with i386 support route chosen by implementation
- ext2 and partition tooling (`e2fsprogs`, `fdisk`/`sfdisk`, loop-mount support)

The implementation plan will pin exact package names and verify they work on the current Ubuntu host.

## Security and Safety Notes

- downloaded source verification is mandatory (SHA256)
- scripts should avoid privileged operations where possible
- if loop devices or mount steps require elevated privileges, scripts must fail with explicit instructions

## Milestone Definition of Done

Linux1 integration is done when all are true:

- `linux-1.0.0/` exists in-repo and is buildable with documented compatibility patches.
- `make linux1` produces a bootable disk image.
- `make test-linux1` passes locally and proves BIOS->MBR->LILO->kernel->init/prompt flow.
- documentation explains fetch/build/run/clean workflow and patch policy.

## Open Risks

- Linux 1.0.0 build compatibility on modern host toolchains may require careful patch scoping.
- LILO install behavior can vary with modern host tooling and may need containerization or loop-device constraints.
- boot timings can vary in CI; smoke tests must use robust timeout and marker matching.

## Risk Mitigations

- keep patch set small and traceable
- stage pipeline validation (kernel build first, then disk assembly, then boot)
- capture complete boot logs for fast diagnosis
- keep smoke tests focused on deterministic markers
