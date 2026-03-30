# Rust Kernel Module Learning

This directory contains Rust kernel modules for learning purposes.

## Important: In-Tree Build Only

Rust kernel modules **cannot** be built out-of-tree (unlike C modules that use `make -C /build M=/path`).
They require access to the kernel's Rust crate artifacts (`libkernel.rmeta`, `libcore.rmeta`, etc.)
that are generated during the kernel build. Therefore, these modules are built as part of the
kernel's `samples/rust/` Kbuild infrastructure.

The `ulk` build script automatically symlinks these files into the kernel source tree.

## Modules

### hello_rust.rs - Hello World
A minimal Rust kernel module demonstrating:
- `module!` macro for metadata (name, license, authors)
- Module parameters (`greeting_count`)
- `pr_info!` kernel logging
- `KVec` (kernel vector) allocation
- `Module` trait for init, `Drop` for cleanup

### rust_chardev.rs - Character Device
A misc character device demonstrating:
- `MiscDevice` registration (creates `/dev/rust-chardev`)
- `ioctl` command handling
- `Mutex`-protected shared state
- User-kernel data transfer (`UserSlice`)
- `InPlaceModule` and pin-init pattern

### rust_sync.rs - Concurrency Primitives
A sync-focused misc device demonstrating:
- fixed-capacity ring-buffer management
- shared state and stats tracking
- blocking producer/consumer discussion points
- lock/condvar trade-offs in Rust kernel code

### rust_workqueue.rs - Deferred Execution
A workqueue sample demonstrating:
- queueing work from a write path
- asynchronous completion accounting
- work-item lifecycle documentation
- unload-time flush/drain behavior

### rust_procfs.rs - /proc Integration
A procfs sample demonstrating:
- `stats` and `config` nodes under `/proc/rust_procfs/`
- seq-style reporting and config parsing
- thin FFI-boundary documentation for procfs/seq_file APIs

### rust_platform.rs - Platform Driver
A simulated platform-driver sample demonstrating:
- probe/remove lifecycle structure
- per-device register state
- userspace register access patterns through a misc device

### rust_slab.rs - Slab Allocator Sample
A slab-oriented misc device demonstrating:
- fixed-size packet objects
- handle tables with generation counters
- allocator statistics and leak-oriented teardown logging

### rust_netdev.rs - Virtual Network Device
A virtual netdev sample demonstrating:
- net-device registration structure
- TX/RX accounting
- carrier state and echo-mode concepts
- companion shell-driven test flow

## Building

These modules are built automatically when you run:

```bash
./ulk kernel=6.19
```

Or build just the Rust samples:

```bash
make rust-samples
```

## Userspace Test Apps

The `user/` directory contains Rust userspace programs that interact with the kernel modules:

### user/test_hello - Hello Module Tester
Checks if the `hello_rust` module is loaded and reads its parameters via sysfs and `/proc/modules`.

### user/test_chardev - Chardev Driver Tester
Opens `/dev/rust-chardev` and exercises the ioctl interface: sends HELLO commands and reads the call counter.

### user/test_sync - Sync Driver Tester
Exercises the ring-buffer push/pop/stats ioctl flow for `/dev/rust-sync`.

### user/test_workqueue - Workqueue Driver Tester
Queues work through the write path and reads the completion count ioctl.

### user/test_platform - Platform Driver Tester
Reads and writes simulated registers through `/dev/rust-platform`.

### user/test_slab - Slab Driver Tester
Allocates packet handles, fills payload bytes, reads stats, and frees handles.

Build the test apps (done automatically by `./ulk`):
```bash
make -C linux-lab/examples/rust/rust_learn/user
```

The `scripts/` directory contains shell helpers for the procfs, platform,
slab, sync, workqueue, and netdev samples. These are intended for the QEMU
guest after the modules are present under `build/samples/rust/`.

## Testing in QEMU Guest

After booting into the QEMU guest (the qulk directory is mounted at `/home/qwert`):

```bash
# Load hello_rust and run its test
insmod /home/qwert/build/samples/rust/hello_rust.ko greeting_count=5
/home/qwert/linux-lab/examples/rust/rust_learn/user/test_hello
dmesg | tail

# Load chardev and run its test
insmod /home/qwert/build/samples/rust/rust_chardev.ko
/home/qwert/linux-lab/examples/rust/rust_learn/user/test_chardev
dmesg | tail

# Load the advanced samples and use either the userspace helpers or scripts
insmod /home/qwert/build/samples/rust/rust_sync.ko
/home/qwert/linux-lab/examples/rust/rust_learn/user/test_sync

insmod /home/qwert/build/samples/rust/rust_workqueue.ko
/home/qwert/linux-lab/examples/rust/rust_learn/user/test_workqueue

insmod /home/qwert/build/samples/rust/rust_procfs.ko
/home/qwert/linux-lab/examples/rust/rust_learn/scripts/test_rust_procfs.sh

insmod /home/qwert/build/samples/rust/rust_platform.ko
/home/qwert/linux-lab/examples/rust/rust_learn/scripts/test_rust_platform.sh

insmod /home/qwert/build/samples/rust/rust_slab.ko
/home/qwert/linux-lab/examples/rust/rust_learn/scripts/test_rust_slab.sh

insmod /home/qwert/build/samples/rust/rust_netdev.ko
/home/qwert/linux-lab/examples/rust/rust_learn/scripts/test_rust_netdev.sh

# Unload
rmmod rust_netdev
rmmod rust_slab
rmmod rust_platform
rmmod rust_procfs
rmmod rust_workqueue
rmmod rust_sync
rmmod rust_chardev
rmmod hello_rust
```

## Adding Your Own Module

1. Create `linux-lab/examples/rust/rust_learn/my_module.rs`
2. Add a `CONFIG_SAMPLE_RUST_MY_MODULE` entry to `linux-lab/examples/rust/rust_learn/Kconfig`
3. Add `obj-$(CONFIG_SAMPLE_RUST_MY_MODULE) += my_module.o` to `linux-lab/examples/rust/rust_learn/Makefile`
4. Add `echo "CONFIG_SAMPLE_RUST_MY_MODULE=m" >> $top/build/.config` to the Rust config section in `ulk`
5. Rebuild: `./ulk kernel=6.19`

## Kernel Rust API Reference

- Kernel Rust docs: `Documentation/rust/` in the kernel source
- Quick start: `Documentation/rust/quick-start.rst`
- Coding guidelines: `Documentation/rust/coding-guidelines.rst`
- In-tree samples: `kernel/linux-6.19/samples/rust/`
- Rust API (build with `make rustdoc`): `build/Documentation/output/rust/rustdoc/kernel/`
