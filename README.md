# QOS

A multi-OS workspace for building and studying operating systems: two custom kernels (C and Rust), Linux 1.0.0, a modern Linux kernel lab, and xv6.

## Repo Structure

OS roots:

| Directory | Description |
|-----------|-------------|
| `c-os/` | Custom OS kernel and userspace in C |
| `rust-os/` | Custom OS kernel and userspace in Rust |
| `linux-1.0.0/` | Linux 1.0.0 build, userspace, and boot pipeline |
| `linux-lab/` | Manifest-driven modern Linux kernel lab |
| `xv6/` | xv6-x86 (MIT teaching OS, i386) |

Shared infrastructure:

| Directory | Description |
|-----------|-------------|
| `scripts/` | Build and boot scripts per OS (grouped by OS under `scripts/<os>/`) |
| `tests/` | Pytest test suite covering all OS roots |
| `qemu/` | QEMU launch scripts and configs |
| `tools/` | Bootstrap probe tools (C and Rust) |
| `docs/` | Specs, plans, and design notes |
| `build/` | All build artifacts (generated, not committed) |

## Prerequisites

Required:

- `make`
- `python3` + `pytest`
- `qemu-system-x86_64`
- `qemu-system-aarch64`

Build toolchains:

- C/x86_64: `x86_64-linux-gnu-as`, `x86_64-linux-gnu-ld`, `x86_64-linux-gnu-objcopy`
- C/aarch64: `aarch64-linux-gnu-gcc`, `aarch64-linux-gnu-ld`
- Rust: `cargo`, `rustup`
- xv6: `i686-linux-gnu-as`, `i686-linux-gnu-ld`, `i686-linux-gnu-gcc`, `i686-linux-gnu-objcopy`

## Quick Start

```bash
# Build and run all smoke tests
make test-all
pytest -q
```

## OS Guides

### c-os

Custom OS kernel in C, targeting x86_64 and aarch64. Boots to an interactive shell under QEMU.

```bash
make c x86_64
make c aarch64
```

Artifacts: `c-os/build/<arch>/`

### rust-os

Custom OS kernel in Rust, targeting x86_64 and aarch64. Same contract as c-os.

```bash
make rust x86_64
make rust aarch64
```

Artifacts: `rust-os/build/<arch>/`

### linux-1.0.0

Linux 1.0.0 build and boot pipeline. Fetches sources, builds the kernel and userspace, assembles a LILO boot image, and runs under QEMU.

```bash
make linux1
```

Build steps are grouped under `scripts/linux1/`. Owned assets live under `linux-1.0.0/`:

- `userspace/` — custom userspace sources
- `manifests/` — source manifest lock
- `patches/` — LILO patch stack

Artifacts: `build/linux1/`

Test:

```bash
make test-linux1
```

### linux-lab

Manifest-driven lab for building and booting modern Linux kernels. Supports profiles, mirrors, multiple architectures, and a broad imported sample set.

Binary locations:

- Native CLI: `linux-lab/bin/linux-lab`
- Compatibility shim: `linux-lab/bin/ulk`

Common workflow:

```bash
# Validate a request
make linux-lab-validate

# Generate a stage plan
make linux-lab-plan

# Dry-run a full workflow
make linux-lab-run
```

Direct CLI usage:

```bash
linux-lab/bin/linux-lab validate \
  --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab

linux-lab/bin/linux-lab plan \
  --kernel 6.9.8 --arch arm64 --image jammy --profile debug --mirror sg

linux-lab/bin/linux-lab run \
  --kernel 6.18.4 --arch x86_64 --image noble --profile default-lab --dry-run
```

`ulk` shim example:

```bash
linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg
```

Profiles:

- `default-lab` — curated subset, the typical starting point
- `full-samples` — expands to the full imported sample set (modules, BPF, Rust, userspace apps)

Artifacts: `build/linux-lab/requests/`

### xv6

xv6-x86 (MIT teaching OS, i386), cross-compiled from an AArch64 host.

```bash
make xv6
```

Test:

```bash
make test-xv6
```

Clean:

```bash
make xv6-clean
```

## Shared Build And Test

Run all smoke targets across all OS roots:

```bash
make test-all
```

Run the full Python test suite:

```bash
pytest -q
```

## Manual QEMU Notes

### C / x86_64

```bash
make c x86_64
./qemu/x86_64.sh c-os/build/x86_64/disk.img
```

Expected serial output includes `QOS kernel started impl=c arch=x86_64` and a `qos-sh:/>` prompt.

### Rust / x86_64

```bash
make rust x86_64
./qemu/x86_64.sh rust-os/build/x86_64/disk.img
```

### C / aarch64

```bash
make c aarch64
truncate -s 64M qemu/configs/c-aarch64.disk.img  # if missing
./qemu/aarch64.sh \
  qemu/configs/c-aarch64.disk.img \
  c-os/build/aarch64/kernel.elf \
  c-os/build/aarch64/initramfs.cpio
```

### Rust / aarch64

```bash
make rust aarch64
truncate -s 64M qemu/configs/rust-aarch64.disk.img  # if missing
./qemu/aarch64.sh \
  qemu/configs/rust-aarch64.disk.img \
  rust-os/build/aarch64/kernel.elf \
  rust-os/build/aarch64/initramfs.cpio
```

Preview a QEMU command without launching:

```bash
./qemu/x86_64.sh --dry-run c-os/build/x86_64/disk.img
./qemu/aarch64.sh --dry-run qemu/configs/c-aarch64.disk.img \
  c-os/build/aarch64/kernel.elf c-os/build/aarch64/initramfs.cpio
```

QEMU keeps running after boot; use `Ctrl+C` to stop it.
