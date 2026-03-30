# Advanced Rust Kernel Module Samples — Design Spec

**Date:** 2026-03-30
**Status:** Approved
**Location:** `linux-lab/examples/rust/rust_learn/`

## Overview

Add 6 advanced Rust kernel modules to linux-lab, progressing from concurrency
primitives through network device drivers. Each module builds on concepts from
the previous ones. All use the existing `kernel-tree-rust` build infrastructure
(symlinked into the kernel tree's `samples/rust/` directory).

**Approach:** Safe Rust APIs where available, targeted `unsafe` FFI wrappers for
subsystems without stable Rust bindings (procfs, slab, netdev). The unsafe blocks
serve as teaching moments about safety boundaries.

## Existing Modules (Context)

| Module | Level | Concepts |
|--------|-------|----------|
| `hello_rust.rs` | Beginner | module! macro, params, KVec, pr_info |
| `rust_chardev.rs` | Intermediate | MiscDevice, ioctl, Mutex, UserSlice, pin_init |
| `rust_minimal.rs` | Beginner | Upstream sample — Vec, Module trait |
| `rust_print.rs` | Beginner | Upstream sample — print macros, Arc |

## New Modules

### Module 1: `rust_sync.rs` — Concurrency Primitives

**Subsystem:** Concurrency & synchronization
**Complexity:** Medium
**Device:** `/dev/rust-sync` (misc device)

**Behavior:**
- Shared ring buffer (fixed capacity, e.g. 64 entries) protected by SpinLock
- Writer ioctl pushes u64 values into the ring buffer
- Reader ioctl pops values, blocks via CondVar when buffer is empty
- Stats tracked via Arc-shared state: push count, pop count, high watermark
- Drop drains and logs remaining items

**Teaching points:**
- SpinLock vs Mutex — when to use which (sleep context vs atomic context)
- CondVar wait/notify pattern in kernel space
- Arc for shared ownership across open file descriptors
- `#[pin_data]` / `pin_init!` patterns for pinned locked structs

**Kconfig:** `CONFIG_SAMPLE_RUST_SYNC`

---

### Module 2: `rust_workqueue.rs` — Deferred Execution

**Subsystem:** Concurrency & scheduling
**Complexity:** Medium
**Device:** `/dev/rust-workqueue` (misc device)

**Behavior:**
- Write to device queues a work item for asynchronous processing
- Each work item logs a message, increments a completion counter
- Ioctl returns count of completed work items
- Module unload flushes all pending work cleanly

**Teaching points:**
- `kernel::workqueue::Work` trait and `impl_has_work!` macro
- System workqueue vs private workqueue allocation
- Arc-prevent-premature-drop pattern for work item lifecycle
- Flush/drain for clean module teardown
- SpinLock protecting shared counter (builds on module 1)

**Kconfig:** `CONFIG_SAMPLE_RUST_WORKQUEUE`

---

### Module 3: `rust_procfs.rs` — Kernel Data via /proc

**Subsystem:** Filesystem (procfs)
**Complexity:** Medium-High
**Interface:** `/proc/rust_procfs/stats` (read-only), `/proc/rust_procfs/config` (read-write)

**Behavior:**
- `stats`: live counters (read count, open count, uptime) via seq_file iteration
- `config`: key-value store (max 16 entries), writable via
  `echo "key=value" > /proc/rust_procfs/config`
- Uses unsafe FFI: `proc_create`, `proc_mkdir`, `remove_proc_subtree`, seq_file ops

**Teaching points:**
- Thin unsafe FFI wrappers around C kernel APIs (`bindings::` imports)
- seq_file iteration protocol: start → next → stop → show
- C function pointer → Rust static vtable pattern
- Mutex-protected mutable state shared between read/write paths
- Proper cleanup via Drop (remove_proc_subtree)

**Kconfig:** `CONFIG_SAMPLE_RUST_PROCFS`

---

### Module 4: `rust_platform.rs` — Platform Device Driver

**Subsystem:** Device drivers
**Complexity:** Medium-High
**Interface:** misc device created on probe, platform device via sysfs

**Behavior:**
- Registers platform driver for `"rust-platform-demo"` device
- Probe: allocates driver-private data, creates misc device, logs resource info
- Ioctl: read/write simulated hardware registers (in-memory u32 array, 16 regs)
- Remove: tears down misc device, frees resources
- Companion script creates/destroys device via
  `/sys/bus/platform/drivers/.../bind|unbind`

**Teaching points:**
- `kernel::platform::Driver` trait (available since ~6.8)
- Driver lifecycle: probe vs init, remove vs drop
- Per-device private data pattern
- Simulated MMIO as stepping stone to real register I/O
- sysfs-triggered device instantiation for testing without hardware

**Kconfig:** `CONFIG_SAMPLE_RUST_PLATFORM`

---

### Module 5: `rust_slab.rs` — Custom Object Cache

**Subsystem:** Memory management
**Complexity:** High
**Device:** `/dev/rust-slab` (misc device)

**Behavior:**
- Wraps `kmem_cache_create` / `kmem_cache_alloc` / `kmem_cache_free` via unsafe FFI
- Cached object type: `Packet` struct (64 bytes — header + payload)
- Ioctls: ALLOC (returns integer handle), FREE (by handle), STATS (active/total/highwater),
  FILL (writes payload into allocated handle)
- Handle table: `KVec<Option<*mut Packet>>` protected by SpinLock
- Module unload: frees all live objects, destroys cache, warns on leaks

**Teaching points:**
- Unsafe FFI for `kmem_cache_*` — the kernel's fast-path allocator
- Object lifetime management: alloc/free pairing, leak detection
- Handle-based API design (integer handles, not pointers — safe userspace boundary)
- Why slab matters: same-size hot-cache vs generic kmalloc
- SpinLock for concurrent access (builds on module 1)

**Kconfig:** `CONFIG_SAMPLE_RUST_SLAB`

---

### Module 6: `rust_netdev.rs` — Virtual Network Device Driver

**Subsystem:** Networking
**Complexity:** High
**Interface:** `rustnet0` network interface

**Behavior:**
- Registers virtual network interface via unsafe FFI:
  `alloc_netdev`, `register_netdev`, `net_device_ops`
- **TX path:** `ndo_start_xmit` receives sk_buffs, increments counters, loops back
  or drops depending on echo-mode flag
- **RX path:** In echo mode, clones sk_buff, swaps src/dst MAC, re-injects via
  `netif_rx` — complete TX→RX round trip visible from userspace
- **Stats:** `ndo_get_stats64` exposes tx/rx packets, bytes, drops
- **Carrier control:** sysfs toggle simulates link up/down
  (`netif_carrier_on` / `netif_carrier_off`)
- Companion test script: `ip link set rustnet0 up`, ping, tcpdump verification

**Teaching points:**
- `net_device` registration lifecycle — largest C-bridge in the series
- sk_buff basics: head/data/tail/end, `skb_clone`, `skb_push`/`skb_pull`
- Why echo mode uses `netif_rx` rather than direct callback (NAPI context)
- `net_device_ops` vtable: mapping C function pointers to Rust methods
- Carrier state machine, queue start/stop
- Real-world verification with ip, ping, tcpdump

**Kconfig:** `CONFIG_SAMPLE_RUST_NETDEV`

---

## Build Integration

All 6 modules are added to:

1. **Source:** `linux-lab/examples/rust/rust_learn/<module>.rs`
2. **Kconfig:** Append to `linux-lab/examples/rust/rust_learn/Kconfig`
3. **Makefile:** Append to `linux-lab/examples/rust/rust_learn/Makefile`
4. **Config fragment:** Add `CONFIG_SAMPLE_RUST_<X>=m` lines to
   `linux-lab/fragments/rust.conf`
5. **Catalog:** Update `linux-lab/catalog/examples/rust_learn.yaml` tags/groups

No new profiles or groups needed — these fit within the existing `rust-core`
and `rust-all` groups.

## Companion Files

Each module includes:
- Detailed doc-comments (`//!`) with usage instructions and expected output
- Inline comments explaining every `unsafe` block's safety argument
- Where applicable, a test script or C userspace helper in
  `linux-lab/examples/rust/rust_learn/user/`

## Dependencies Between Modules

```
rust_sync ──┬── rust_workqueue
             │
             ├── rust_procfs
             │
             ├── rust_slab
             │
             └── rust_netdev (uses spinlock, workqueue concepts,
                              exposes stats via /proc pattern)

rust_platform (standalone, uses misc device from rust_chardev)
```

Modules are independent at build time (no compile-time deps). The dependency
is conceptual — later modules assume familiarity with patterns from earlier ones.

## Target Kernels

All modules target kernels with `CONFIG_RUST=y` support. Module 4
(`rust_platform`) requires kernel ≥ 6.8 for `kernel::platform` bindings.
Others work on any Rust-enabled kernel (≥ 6.1).

## Testing Strategy

Each module is verified by:
1. `insmod` / `rmmod` — clean load/unload without warnings
2. `dmesg` — expected log messages appear
3. Companion script or manual steps exercising the userspace interface
4. No memory leaks (check `/proc/slabinfo`, `kmemleak` if enabled)
