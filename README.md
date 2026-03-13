# QOS

QOS is a dual-implementation OS workspace:

- `c-os/`: C implementation
- `rust-os/`: Rust implementation

The current repository focuses on a testable scaffold with kernel/libc/userspace contracts and QEMU smoke flows.

## Prerequisites

Required tools:

- `make`
- `python3` + `pytest`
- `qemu-system-x86_64`
- `qemu-system-aarch64`

Build toolchains:

- C/x86_64 image build: `x86_64-linux-gnu-as`, `x86_64-linux-gnu-ld`, `x86_64-linux-gnu-objcopy`
- C/aarch64 kernel build: `aarch64-linux-gnu-gcc`, `aarch64-linux-gnu-ld`
- Rust builds: `cargo`, `rustup`

## Quick Build

From repo root:

```bash
# C
make c x86_64
make c aarch64

# Rust
make rust x86_64
make rust aarch64
```

Run all smoke targets:

```bash
make test-all
```

Run full Python test suite:

```bash
pytest -q
```

## Manual QEMU Testing

All commands below are run from repo root.

### C / x86_64

```bash
make c x86_64
./qemu/x86_64.sh c-os/build/x86_64/disk.img
```

Expected serial output includes:

- `QOS kernel started impl=c arch=x86_64`
- `qos-sh:/>` prompt

Example interactive session:

```text
init[1]: starting /sbin/init
init[1]: exec /bin/sh
qos-sh:/> help
qos-sh commands:
  help
  echo <text>
  ps
  ping <ip>
  ip addr
  ip route
  wget <url>
  exit
qos-sh:/> ping 10.0.2.2
PING 10.0.2.2
64 bytes from 10.0.2.2: icmp_seq=1 ttl=64 time=1ms
1 packets transmitted, 1 received
qos-sh:/> exit
logout
```

QEMU keeps running after boot (and after `exit`); use `Ctrl+C` to stop it.

### Rust / x86_64

```bash
make rust x86_64
./qemu/x86_64.sh rust-os/build/x86_64/disk.img
```

Expected serial output includes:

- `QOS kernel started impl=rust arch=x86_64`

### C / aarch64

Build artifacts:

```bash
make c aarch64
```

Create a disk image for virtio-blk (if missing):

```bash
truncate -s 64M qemu/configs/c-aarch64.disk.img
```

Run QEMU:

```bash
./qemu/aarch64.sh \
  qemu/configs/c-aarch64.disk.img \
  c-os/build/aarch64/kernel.elf \
  c-os/build/aarch64/initramfs.cpio
```

Expected serial output includes:

- `QOS kernel started impl=c arch=aarch64`
- `qos-sh:/>` prompt

### Rust / aarch64

Build artifacts:

```bash
make rust aarch64
```

Create a disk image for virtio-blk (if missing):

```bash
truncate -s 64M qemu/configs/rust-aarch64.disk.img
```

Run QEMU:

```bash
./qemu/aarch64.sh \
  qemu/configs/rust-aarch64.disk.img \
  rust-os/build/aarch64/kernel.elf \
  rust-os/build/aarch64/initramfs.cpio
```

Expected serial output includes:

- `QOS kernel started impl=rust arch=aarch64`
- `qos-sh:/>` prompt

### In-Guest Commands

Current shell features are split by target:

- x86_64 probe shells: `help`, `echo`, `ps`, `ping`, `ip`, `wget`, `exit`
- aarch64 C probe shell: adds `ls`, `cat`, `touch`, `edit`, `insmod`, `rmmod`, `wqdemo`, `mapwatch on|off|side on|off`, plus shell operators `<`, `>`, `>>`, `|`
- aarch64 Rust probe shell: supports `ls`, `pwd`, `cat`, `touch`, `edit`, `insmod`, `rmmod`, `wqdemo`, `mapwatch on|off|side on|off` in addition to `help`, `echo`, `ps`, `ping`, `ip`, `wget`, `exit`

For C/aarch64, each `/bin/*` command image is built from source in
`tools/aarch64-c-probe/cmd-src/*.c` and embedded as ELF bytes by
`scripts/build_aarch64_cmd_elves.sh` during `make c aarch64`.

Both `make c <arch>` and `make rust <arch>` (`arch = x86_64|aarch64`) now also
emit per-command ELF files on disk at:

- `c-os/build/x86_64/rootfs/bin/`
- `c-os/build/aarch64/rootfs/bin/`
- `rust-os/build/x86_64/rootfs/bin/`
- `rust-os/build/aarch64/rootfs/bin/`

Source-built test plugin artifacts are also generated at build time:

- `c-os/build/<arch>/rootfs/lib/libqos_test.so`
- `c-os/build/<arch>/rootfs/lib/modules/qos_test.ko`
- `rust-os/build/<arch>/rootfs/lib/libqos_test.so`
- `rust-os/build/<arch>/rootfs/lib/modules/qos_test.ko`

`edit` command usage:

- `edit /tmp/note hello qos` (replace file content)
- `echo "line" | edit /tmp/note` (replace from pipeline input)

Redirection examples:

```text
echo hello > /tmp/msg
cat < /tmp/msg
echo appended >> /tmp/msg
echo pipe-data | cat
```

Proc-like kernel info files (via `cat`):

- `/proc/meminfo`
- `/proc/kernel/status`
- `/proc/net/dev`
- `/proc/syscalls`
- `/proc/uptime`
- `/proc/runtime/map`
- `/proc/<pid>/status`

Live runtime map watcher (SSH-safe, keeps prompt active):

- `mapwatch on`
- `mapwatch off`
- `mapwatch side on` (writes live `/proc/runtime/map` snapshots to host-side stream)
- `mapwatch side off`

Side-stream usage (no shell redraw, good for split SSH terminals):

```bash
# terminal A: run QEMU (shell stays interactive on UART)
QOS_MAPWATCH_LOG=/tmp/qos-mapwatch.log ./qemu/aarch64.sh ...

# terminal B: follow live map snapshots
tail -f /tmp/qos-mapwatch.log
```

## Shared Libraries And Hot-Reload Modules

Both C and Rust kernels now expose shared-object and module syscalls:

- `36`: `dlopen(path, flags)` -> handle
- `37`: `dlclose(handle)` -> `0` on success
- `38`: `dlsym(handle, name)` -> symbol address token
- `39`: `modload(path)` -> module id
- `40`: `modunload(module_id)` -> `0` on success
- `41`: `modreload(module_id)` -> new generation number

Behavior implemented in the scaffold:

- `dlopen/modload` accept only ELF `ET_DYN` images (shared objects).
- `dlopen` deduplicates by path and uses refcounting.
- `modreload` keeps module id stable and increments generation.
- `modunload` releases module state and backing shared object reference.

Userspace wrappers are available in both libc implementations:

- C libc: `qos_dlopen`, `qos_dlclose`, `qos_dlsym`, `qos_modload`, `qos_modunload`, `qos_modreload`
- Rust libc: same `qos_*` API names

Shell commands for module lifecycle and workqueue demo:

- `insmod /lib/modules/qos_test.ko`
- `rmmod /lib/modules/qos_test.ko` (or `rmmod <id>`)
- `wqdemo` (enqueues two work items and drains them via softirq)

## Useful QEMU Options

`qemu/aarch64.sh` supports:

- Packet capture:

```bash
QOS_PCAP_PATH=build/aarch64/manual.pcap ./qemu/aarch64.sh ...
```

- UDP host forward:

```bash
QOS_HOSTFWD_UDP_HOST_PORT=5555 \
QOS_HOSTFWD_UDP_GUEST_PORT=5555 \
./qemu/aarch64.sh ...
```

- Side mapwatch stream destination (set `off` to disable):

```bash
QOS_MAPWATCH_LOG=/tmp/qos-mapwatch.log ./qemu/aarch64.sh ...
QOS_MAPWATCH_LOG=off ./qemu/aarch64.sh ...
```

Preview generated command without launching:

```bash
./qemu/x86_64.sh --dry-run c-os/build/x86_64/disk.img
./qemu/aarch64.sh --dry-run qemu/configs/c-aarch64.disk.img c-os/build/aarch64/kernel.elf c-os/build/aarch64/initramfs.cpio
```

## Notes

- Current smoke checks are marker-based boot/runtime probes over serial.
- aarch64 startup stubs now perform an explicit EL2→EL1 transition path (when entered at EL2) and preserve DTB handoff register state into kernel entry.
- aarch64 probe boot currently enters with `qemu -kernel` and may not receive a nonzero DTB pointer in `x0` on this ELF entry path.
- To keep handoff markers DTB-derived in this mode, both C and Rust aarch64 probes build a valid embedded fallback FDT (`/memory` + `/chosen` initrd fields) and parse it through the same DTB parser path.
- This improves boot-info fidelity (`mmap_source=dtb`, `initramfs_source=dtb`) but is still not a full implementation of the design-doc bootloader sequence.
- If QEMU seems stuck, stop with `Ctrl+C`.
