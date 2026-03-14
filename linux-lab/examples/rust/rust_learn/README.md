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

Build the test apps (done automatically by `./ulk`):
```bash
make -C modules/rust_learn/user
```

## Testing in QEMU Guest

After booting into the QEMU guest (the qulk directory is mounted at `/home/qwert`):

```bash
# Load hello_rust and run its test
insmod /home/qwert/build/samples/rust/hello_rust.ko greeting_count=5
/home/qwert/modules/rust_learn/user/test_hello
dmesg | tail

# Load chardev and run its test
insmod /home/qwert/build/samples/rust/rust_chardev.ko
/home/qwert/modules/rust_learn/user/test_chardev
dmesg | tail

# Unload
rmmod rust_chardev
rmmod hello_rust
```

## Adding Your Own Module

1. Create `modules/rust_learn/my_module.rs`
2. Add a `CONFIG_SAMPLE_RUST_MY_MODULE` entry to `modules/rust_learn/Kconfig`
3. Add `obj-$(CONFIG_SAMPLE_RUST_MY_MODULE) += my_module.o` to `modules/rust_learn/Makefile`
4. Add `echo "CONFIG_SAMPLE_RUST_MY_MODULE=m" >> $top/build/.config` to the Rust config section in `ulk`
5. Rebuild: `./ulk kernel=6.19`

## Kernel Rust API Reference

- Kernel Rust docs: `Documentation/rust/` in the kernel source
- Quick start: `Documentation/rust/quick-start.rst`
- Coding guidelines: `Documentation/rust/coding-guidelines.rst`
- In-tree samples: `kernel/linux-6.19/samples/rust/`
- Rust API (build with `make rustdoc`): `build/Documentation/output/rust/rustdoc/kernel/`
