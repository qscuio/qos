# QOS — Dual-Implementation OS Design Spec

**Date:** 2026-03-11
**Status:** Approved (Revision 3)

---

## Overview

QOS is a monorepo containing two independent, feature-equivalent operating system
implementations: one in pure C (`c-os/`), one in pure Rust (`rust-os/`). Both target
x86-64 and AArch64 as primary architectures (x86 32-bit and ARM 32-bit are secondary).
The project is tested exclusively via QEMU. The goal is a modest, real OS: virtual
memory, preemptive scheduling, VFS, network stack, and a working userspace with a shell.

---

## Repository Structure

```
qos/
├── Makefile                        # Top-level: delegates to c-os/ and rust-os/
├── qemu/
│   ├── x86_64.sh                   # QEMU launch scripts
│   ├── aarch64.sh
│   └── configs/                    # Machine configs, disk images
├── docs/
│   └── superpowers/specs/
├── c-os/
│   ├── Makefile
│   ├── boot/
│   │   ├── x86_64/                 # 16-bit stub → protected → long mode
│   │   └── aarch64/                # EL2→EL1 setup, DTB parse, MMU init
│   ├── kernel/
│   │   ├── arch/x86_64/            # IDT, APIC, syscall entry, context switch
│   │   ├── arch/aarch64/           # Exception vectors, GIC, syscall entry
│   │   ├── mm/                     # PMM, VMM, slab allocator
│   │   ├── sched/                  # Scheduler, context switch
│   │   ├── fs/                     # VFS, initramfs, tmpfs, procfs, ext2
│   │   ├── net/                    # Ethernet, IPv4, ARP, UDP, TCP, sockets
│   │   └── drivers/                # e1000, virtio-net (MMIO), uart, vga
│   ├── libc/                       # Minimal C standard library
│   └── userspace/                  # Shell + sample programs
└── rust-os/
    ├── Cargo.toml                  # Workspace
    ├── boot/                       # x86_64 + aarch64 bootloaders
    ├── kernel/                     # Kernel crate (no_std)
    ├── libc/                       # Minimal libc crate
    └── userspace/                  # Shell + sample programs
```

**Build targets:**
- `make c x86_64` / `make c aarch64`
- `make rust x86_64` / `make rust aarch64`
- `make test-all` — builds all 4 targets and runs QEMU smoke tests

---

## Disk Layout (x86-64)

The raw disk image uses a flat sector layout — no partition table inside the bootloader
itself. Sizes are generous to avoid revisiting.

| Region      | LBA Start | Size      | Notes |
|-------------|-----------|-----------|-------|
| Stage 1     | 0         | 1 sector  | MBR (512 bytes) |
| Stage 2     | 1         | 127 sectors | Max ~63 KB; fits in low memory |
| Kernel ELF  | 128       | 1024 sectors | Max ~512 KB |
| initramfs   | 1152      | 4096 sectors | Max ~2 MB CPIO archive |

Stage 1 reads Stage 2 using BIOS int 13h (CHS or LBA extension). Stage 2 reads
the kernel ELF header, then loads PT_LOAD segments. Stage 2 then reads the initramfs
and passes its physical address + size in `boot_info`.

AArch64 has no disk bootloader: QEMU `-kernel` loads the kernel ELF directly.
The initramfs is passed via QEMU `-initrd` and its address/size are in the DTB
under `/chosen/linux,initrd-start` and `/chosen/linux,initrd-end`.

---

## boot_info Structure

Both bootloaders (x86-64 and AArch64) pass a single `boot_info` pointer to `kernel_main`.
The struct is defined in a shared header (`boot/boot_info.h` / `boot/boot_info.rs`):

```c
// C version (naturally aligned, no padding issues)
typedef struct {
    uint64_t magic;               // 0x514F53424F4F5400 ("QOSBOOT\0")

    // Memory map
    uint32_t mmap_entry_count;
    uint32_t _pad0;
    struct mmap_entry {
        uint64_t base;
        uint64_t length;
        uint32_t type;            // 1=usable, 2=reserved, 3=ACPI, 4=NVS, 5=bad
        uint32_t _pad;
    } mmap_entries[128];

    // Framebuffer (x86-64 VGA / AArch64 virtio-gpu or absent)
    uint64_t fb_addr;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;            // bytes per row
    uint8_t  fb_bpp;              // bits per pixel
    uint8_t  _pad1[3];

    // initramfs
    uint64_t initramfs_addr;      // physical address
    uint64_t initramfs_size;      // bytes

    // ACPI (x86-64) / DTB (AArch64)
    uint64_t acpi_rsdp_addr;      // 0 if not present
    uint64_t dtb_addr;            // 0 if not present
} boot_info_t;
```

The Rust version is `#[repr(C)]` with identical field layout.

---

## Bootloader

Both implementations follow the same staged boot sequence per architecture.
Stage 1 for x86-64 is a small `.asm` file (MBR, unavoidable). All subsequent
stages are pure C or pure Rust (`#![no_std]`).

### x86-64

```
Stage 1 (512 bytes, MBR, assembly)
  - BIOS loads to 0x7C00, 16-bit real mode
  - Enable A20 line (port 0x92 fast method + BIOS int 15h fallback)
  - Use BIOS int 15h E820 to build memory map (stored at 0x8000, max 128 entries)
  - Load Stage 2 from LBA 1..127 via BIOS int 13h (LBA extension, fallback CHS)
  - Jump to Stage 2 at 0x9000

Stage 2 (C / Rust, loaded at 0x9000)
  - Enter 32-bit protected mode (set up GDT with flat 32-bit CS/DS)
  - Enable PAE paging: set up PML4 + PDPT + PD identity-mapping first 4 GB
  - Enable 64-bit long mode (set EFER.LME, load 64-bit GDT)
  - Read kernel ELF from LBA 128 using in-protected-mode disk I/O (port I/O ATA PIO)
  - Map kernel PT_LOAD segments at their link addresses (0xFFFFFFFF80000000+)
  - Read initramfs from LBA 1152
  - Build boot_info at 0x7000 (below Stage 1, safe in low memory)
  - Jump to kernel_main(boot_info*) in long mode

Memory map reserved regions marked in PMM at kernel init:
  0x00000000-0x00000FFF  (real mode IVT + BDA)
  0x0009F000-0x000FFFFF  (EBDA + VGA + ROM)
  Stage 1/2 load areas, kernel image PA range, initramfs PA range
```

**QEMU command:**
```sh
qemu-system-x86_64 -drive file=disk.img,format=raw,if=ide \
  -device e1000,netdev=net0 -netdev user,id=net0 \
  -serial stdio -display none -m 256M
```

### AArch64

```
Stage 1 (assembly, linked as ELF, QEMU -kernel loads it)
  - CPU starts at EL2 (hypervisor exception level)
  - Zero BSS
  - Disable EL2 traps, configure HCR_EL2 for EL1 operation
  - Drop to EL1 via ERET with synthetic SPSR_EL2
  - Configure MAIR_EL1 (Normal cacheable + Device-nGnRnE)
  - Configure TCR_EL1: 48-bit VA, 4 KB granule, TTBR0=user, TTBR1=kernel
  - Build initial page tables: identity map RAM, map kernel at high address
  - Enable MMU (SCTLR_EL1.M=1)
  - Parse QEMU Device Tree Blob (DTB) at address passed in x0 by QEMU:
      /memory: extract base + size → boot_info mmap_entries (type=usable)
      /chosen/linux,initrd-start+end: initramfs location → boot_info
  - Mark DTB region, initramfs region, and kernel image as reserved in mmap
  - Jump to kernel_main(boot_info*)

GIC version: GICv2 (QEMU virt default). GICD base: 0x08000000, GICC base: 0x08010000.
Generic Timer: EL1 physical timer, IRQ 30 (PPI).
```

**QEMU command:**
```sh
qemu-system-aarch64 -machine virt -cpu cortex-a57 \
  -kernel boot.elf -initrd initramfs.cpio \
  -netdev user,id=net0 -device virtio-net-device,netdev=net0 \
  -serial stdio -display none -m 256M
```

---

## Kernel Architecture

**Type:** Monolithic kernel with clear subsystem boundaries.

### Memory Layout (x86-64)

```
0x0000_0000_0000_0000 - 0x0000_7FFF_FFFF_FFFF  User space (128 TB)
0xFFFF_8000_0000_0000 - 0xFFFF_8000_3FFF_FFFF  Physical memory direct map (1 GB)
0xFFFF_C000_0000_0000 - 0xFFFF_C000_FFFF_FFFF  Kernel heap (1 GB, extendable)
0xFFFF_FFFF_8000_0000 - 0xFFFF_FFFF_FFFF_FFFF  Kernel text/data/stack (2 GB)
```

**AArch64 virtual address layout (TTBR1_EL1 — kernel, 48-bit VA):**

```
0xFFFF_0000_0000_0000 - 0xFFFF_003F_FFFF_FFFF  Physical memory direct map (256 GB max)
0xFFFF_8000_0000_0000 - 0xFFFF_8000_FFFF_FFFF  Kernel heap (1 GB)
0xFFFF_FFFF_8000_0000 - 0xFFFF_FFFF_FFFF_FFFF  Kernel text/data/stack (2 GB link address)
```

`TTBR0_EL1` covers user space (low addresses, same 128 TB range as x86-64).
3-level page tables (4 KB granule, 48-bit VA). Kernel binary linked at
`0xFFFF_FFFF_8000_0000`; direct map and heap are in a completely separate range.

### Memory Manager

**Physical Memory Manager (PMM):**
```
- Bitmap allocator: 1 bit per 4 KB page
- Initialized from boot_info mmap_entries:
    - Mark all pages as reserved (bitmap = all 1s)
    - For each usable mmap_entry: mark pages as free
    - Re-mark as reserved: BIOS/DTB regions, kernel image PA range,
      initramfs PA range, boot_info struct page, PMM bitmap itself
- Interface: page_t pmm_alloc(); void pmm_free(page_t);
- page_t is a physical address (uint64_t)
- Protected by a single spinlock (single-core, so low contention)
```

**AArch64 memory map source:** DTB `/memory` node, parsed by bootloader and
stored in `boot_info.mmap_entries` before kernel starts. No BIOS on AArch64.

**Virtual Memory Manager (VMM):**
```
- Operates on a given address space (page table root = CR3 / TTBR value)
- Interface:
    void  vmm_map(as_t* as, vaddr_t va, page_t pa, uint32_t flags);
    void  vmm_unmap(as_t* as, vaddr_t va);
    page_t vmm_translate(as_t* as, vaddr_t va);
- flags: VM_READ | VM_WRITE | VM_EXEC | VM_USER | VM_COW
- Copy-on-write: VM_COW pages fault on write → pmm_alloc + copy + remap RW
- Kernel VAS operations use the kernel's own as_t (always mapped in TLB)
- Protected by per-address-space spinlock
```

**Kernel Heap (slab allocator):**
```
- Size classes: 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 bytes
- Each size class: linked list of slabs (one or more pages from VMM)
- Objects within a slab: free list threaded through the object space
- Large allocations (> 4096 bytes): direct VMM call, tracked in a small table
- Interface: void* kmalloc(size_t); void kfree(void*);
- Protected by per-size-class spinlock
- Rust: implements core::alloc::GlobalAlloc → enables use of alloc crate
```

---

## Interrupt & Exception Infrastructure

This subsystem is implemented before all others (scheduler, drivers, network depend on it).

### x86-64

```
IDT: 256 entries, loaded via LIDT
  - Entries 0-31:  CPU exceptions (divide by zero, page fault, GPF, etc.)
  - Entries 32-47: IRQ0-IRQ15 (remapped from PIC, or from APIC if initialized)
  - Entry 128:     Syscall software interrupt (INT 0x80, legacy fallback)
  - Entries 49-255: available for drivers

APIC: QEMU emulates APIC. Disable legacy PIC (mask all, remap to 0xF0+).
  - Enable LAPIC (base at APIC_BASE MSR, or MMIO 0xFEE00000)
  - Timer: LAPIC timer, divide-by-16, periodic mode → IRQ 32 for scheduling

IST: Interrupt Stack Table used for stack switch on #DF (double fault) and #NMI.
  Each CPU has a TSS with ist[0] pointing to a dedicated 4 KB emergency stack.

IRQ registration:
  void irq_register(int irq, irq_handler_t handler, void* ctx);
  irq_handler_t: void (*)(int irq, void* ctx, registers_t* regs)
```

### AArch64

```
Exception vectors: 4 KB-aligned table loaded into VBAR_EL1
  - 16 entries: Sync/IRQ/FIQ/SError × {EL1t, EL1h, EL0-64, EL0-32}
  - EL0 Sync → syscall handler (SVC #0) or data/instruction abort
  - EL1h IRQ → IRQ dispatch (scheduler tick, device IRQs)

GIC v2:
  - GICD (distributor) at 0x08000000: enable IRQs, set targets, priorities
  - GICC (CPU interface) at 0x08010000: read IAR to get IRQ#, write EOIR to ack
  - Generic Timer PPI (IRQ 30): EL1 physical timer → scheduling tick
  - virtio-net IRQ: SPI 1 (QEMU virt default)

IRQ registration: same interface as x86-64 (irq_register / irq_handler_t)
```

### Panic

```
void panic(const char* fmt, ...) __attribute__((noreturn));
  - Print message to serial (always works, no locks)
  - Disable interrupts (CLI / MSR DAIF)
  - Halt (HLT loop / WFI loop)

On kernel page fault with no registered handler: panic("kernel page fault @ %p")
```

---

## Scheduling & Threading

**Scheduler:** Preemptive round-robin with 4 priority levels (0=idle, 1=low, 2=normal, 3=high).
Scheduling quantum: 10 ms (100 Hz timer tick). Higher priorities are always served first;
within a level, round-robin. O(1) dispatch: one run queue (doubly-linked list) per priority level.

### Data Structures

```c
// Thread control block
typedef struct thread {
    uint64_t    id;
    struct process* proc;         // owning process
    thread_state_t  state;        // Running|Ready|Blocked|Zombie
    int         priority;         // 0..3
    void*       kernel_stack;     // 8 KB kernel stack (kmalloc'd)
    cpu_context_t ctx;            // saved register context (see below)
    struct thread* next;          // run queue linkage
} thread_t;

// Process control block
typedef struct process {
    uint64_t    pid;
    as_t*       as;               // virtual address space
    fd_table_t  fds;              // file descriptor table [256]
    thread_t*   threads;          // linked list of threads
    struct process* parent;
    int         exit_code;
} process_t;

// x86-64 saved context (pushed by context_switch.asm)
typedef struct { uint64_t r15,r14,r13,r12,rbp,rbx,rip,rsp; } cpu_context_t;
// AArch64 saved context
typedef struct { uint64_t x19..x28, fp, lr, sp, elr, spsr; } cpu_context_t;
```

**Run queue:** `thread_t* run_queues[4]` (one doubly-linked list per priority).
Protected by a global scheduler spinlock (single-core).

**Scheduling quantum:** 10 ms. Timer IRQ decrements `current_thread->ticks_remaining`;
on zero, sets a `need_reschedule` flag, cleared at IRQ return by calling `schedule()`.

### Context Switch

```
x86-64:   Timer IRQ → save callee-saved GPRs to thread->ctx
          → pick next thread from highest non-empty run queue
          → restore next thread's ctx → switch kernel stack (TSS.RSP0)
          → iretq to next thread's saved RIP/RSP

AArch64:  Timer IRQ (EL1 IRQ vector) → save x19-x28, fp, lr, sp, ELR_EL1, SPSR_EL1
          → pick next thread → restore → ERET
```

Entry points (`context_switch_to(thread_t* from, thread_t* to)`) are in `.S` files;
the C/Rust scheduling logic calls them after selecting the next thread.

### Syscall Interface

```
x86-64:   SYSCALL instruction (fast path); SYSRET to return
AArch64:  SVC #0; ERET to return

Syscall convention (both): number in first reg (rax/x8), args in next 6 regs,
return value in rax/x0, error as negative errno.

Syscalls:
  0  fork()         → pid_t
  1  exec(path, argv, envp)
  2  exit(code)
  3  getpid()
  4  wait(pid, status*)
  5  open(path, flags, mode)
  6  read(fd, buf, len)
  7  write(fd, buf, len)
  8  close(fd)
  9  dup2(oldfd, newfd)
  10 mmap(addr, len, prot, flags, fd, off)
  11 munmap(addr, len)
  12 yield()
  13 sleep(ms)
  14 socket(domain, type, proto)
  15 bind(fd, addr*, addrlen)
  16 listen(fd, backlog)
  17 accept(fd, addr*, addrlen*)
  18 connect(fd, addr*, addrlen)
  19 send(fd, buf, len, flags)
  20 recv(fd, buf, len, flags)
  21 stat(path, stat*)
  22 mkdir(path, mode)
  23 unlink(path)
  24 getdents(fd, buf, len)
  25 lseek(fd, offset, whence)
  26 pipe(fds[2])              → creates read/write fd pair
  27 chdir(path)
  28 getcwd(buf, size)
```

### Synchronization Primitives (kernel-internal)

```
spinlock_t:  atomic test-and-set; irq_save/restore variant for ISR use
mutex_t:     sleep-based; waiting threads placed on a wait queue, woken by mutex_unlock
semaphore_t: counter + wait queue; sem_wait / sem_post
```

---

## exec() Kernel Implementation

When a thread calls `exec(path, argv, envp)`:

1. Load and validate target ELF (ET_EXEC or ET_DYN)
2. Destroy current process's VAS (unmap all user mappings, free pages)
3. Create fresh VAS; map ELF PT_LOAD segments with correct permissions
4. If ELF has `PT_INTERP`: load the dynamic linker (`/lib/ld.so`) into VAS at a
   high user address; entry point becomes `ld.so` entry, not program entry
5. Set up initial user stack at `0x0000_7FFF_FFFF_0000` (grows downward in memory).
   Stack layout at entry (higher addresses at top):

   ```
   higher addresses
   ┌─────────────────────────────┐
   │  envp strings (raw bytes)   │
   │  argv strings (raw bytes)   │
   ├─────────────────────────────┤
   │  AT_NULL (auxv terminator)  │
   │  auxv entries (key/val u64) │
   │    AT_PAGESZ = 4096         │
   │    AT_BASE   = ld.so base   │
   │    AT_ENTRY  = prog entry   │
   │    AT_PHENT  = phdr size    │
   │    AT_PHNUM  = phdr count   │
   │    AT_PHDR   = phdr VA      │
   ├─────────────────────────────┤
   │  NULL  (envp terminator)    │
   │  envp[n-1] ptr              │
   │  ...                        │
   │  envp[0] ptr                │
   ├─────────────────────────────┤
   │  NULL  (argv terminator)    │
   │  argv[argc-1] ptr           │
   │  ...                        │
   │  argv[0] ptr                │
   ├─────────────────────────────┤
   │  argc  (uint64_t)           │  ← RSP / SP points here at entry
   └─────────────────────────────┘
   lower addresses
   ```
6. Set thread's saved PC = ELF entry (or ld.so entry if dynamic)
7. Set thread's saved SP = initial stack pointer (pointing to `argc`)
8. SYSRET/ERET to user mode

---

## VFS & Filesystem

### VFS Layer

```
vfs_open() / vfs_read() / vfs_write() / vfs_close()
vfs_mkdir() / vfs_readdir() / vfs_stat() / vfs_unlink()
      ↓
  inode cache (hash table, 256 buckets, LRU eviction)
  dentry cache (path → inode mapping, 512 entries, LRU)
      ↓
  filesystem driver (registered via vfs_mount())
```

**Inode cache:** Write-through for ext2 (writes go to disk immediately via the
driver's `write_inode` callback). Cache entries hold a reference count; eviction
only occurs when refcount == 0. After `fork()`, both parent and child share inode
references (refcount incremented).

**Dentry cache:** Negative entries (path not found) are cached with type=NEGATIVE,
invalidated on `mkdir`/`unlink` in the same directory.

### Concrete Filesystems

| Filesystem | Mount Point | Notes |
|------------|-------------|-------|
| initramfs  | `/`         | CPIO newc format, embedded. Read-only. Contains shell + initial binaries. |
| tmpfs      | `/tmp`      | In-memory. Backed by kmalloc'd pages. Not persistent. |
| procfs     | `/proc`     | Dedicated driver (not tmpfs). Provides `/proc/<pid>/status`, `/proc/meminfo`. |
| ext2       | `/data`     | On QEMU virtio-blk disk image. Writable. Write-through cache. |

### initramfs Format & Embedding

- Format: **newc** (SVR4 without CRC, magic `070701`)
- Embedding: linked into kernel ELF as a section (`__initramfs_start` / `__initramfs_end`
  symbols) via objcopy or a linker script `.incbin` directive
- AArch64: passed via QEMU `-initrd`; address/size from DTB `/chosen` node
- The initramfs driver walks CPIO records at boot, builds an in-memory inode tree,
  then the VFS mounts it as root before executing `/bin/sh`

### File Descriptors

Per-process array: `fd_entry_t fds[256]` where:
```c
typedef struct {
    inode_t*  inode;
    uint64_t  offset;
    uint32_t  flags;    // bit 0: FD_CLOEXEC (= 1); bits 1+: open mode flags
} fd_entry_t;
```

`FD_CLOEXEC = 1` (bit 0 of `flags`). There is no `fcntl` syscall and no `O_CLOEXEC`
open flag — `FD_CLOEXEC` is never set in this implementation. All file descriptors
inherit across `exec()`. The `FD_CLOEXEC` bit is reserved in the struct for future use
and the exec() path checks it (closes if set), but no current code path sets it.

All fds are inherited across `fork()` (refcount on inodes incremented). `dup2(old, new)`
closes `new` if open, then copies the entry.

**Rust:** VFS defined as `trait Filesystem` + `trait Inode` with `dyn` dispatch.
Each filesystem is a separate module. Unsafe is isolated to raw disk I/O.

---

## Network Stack & Drivers

### Drivers

| Architecture | NIC        | QEMU flag | Bus |
|--------------|------------|-----------|-----|
| x86-64       | e1000      | `-device e1000` | PCI |
| AArch64      | virtio-net | `-device virtio-net-device` | MMIO |

**e1000 (PCI, x86-64):**
- Enumerate PCI bus via port I/O (CONFIG_ADDRESS 0xCF8 / CONFIG_DATA 0xCFC)
- Find device 8086:100E, read BAR0 (MMIO base)
- Initialize TX/RX descriptor rings (16 descriptors each, 2 KB buffers)
- Register IRQ via `irq_register(e1000_irq, e1000_handler, dev)`

**virtio-net (MMIO, AArch64):**
- MMIO base and IRQ: discovered from DTB `/virtio_mmio@...` nodes. Each node
  has a `reg` property (base address + size) and an `interrupts` property (SPI #).
  Probe all nodes, check `MagicValue` register (must be `0x74726976`), then
  `DeviceID` register (must be `1` for network). Use the first matching node.
- Virtio MMIO buses on QEMU virt start at `0x0A000000` with 0x200-byte stride;
  the exact slot varies by QEMU version — always use DTB discovery, not a hardcoded address.
- Initialization sequence: magic check → version check → DeviceID → reset →
  set ACKNOWLEDGE + DRIVER status bits → negotiate features (VIRTIO_NET_F_MAC) →
  set FEATURES_OK → set up virtqueues (RX queue 0, TX queue 1, 16 descriptors each,
  split-queue format) → set DRIVER_OK
- Virtqueue: split-queue format, 16 descriptors each

**UART (serial console, both architectures):**
- x86-64: 16550 at I/O port 0x3F8, 115200 baud, polling for early output, IRQ-driven after
- AArch64: PL011 at 0x09000000 (QEMU virt), same approach

**VGA text mode (x86-64 only):** MMIO at 0xB8000, 80×25 cells, used for early
debug output before serial is initialized.

### Network Stack Layers

```
Application  (socket/bind/listen/accept/connect/send/recv syscalls)
     ↓
Socket layer  — socket structs, per-socket RX/TX buffers (4 KB ring each),
                port allocation table (bitmap, ports 1024-65535)
     ↓
TCP           — full connection state machine (CLOSED/LISTEN/SYN_SENT/
                SYN_RECEIVED/ESTABLISHED/FIN_WAIT_1/FIN_WAIT_2/CLOSE_WAIT/
                CLOSING/LAST_ACK/TIME_WAIT)
                Sliding window: send window tracks peer's advertised window.
                Receive window: 8 KB fixed.
                Retransmit: single retransmit timer per connection, 500 ms
                initial RTO with exponential backoff (max 60 s), up to 5 retries.
                No congestion control (no CWND/SSTHRESH) — out of scope.
                Passive open: listen()/accept() fully supported.
UDP           — demux by (dst_ip, dst_port); no reliability
     ↓
IPv4          — static routing: default gateway 10.0.2.2, local subnet 10.0.2.0/24
                ARP cache: 32-entry table; ARP request/reply; 30 s TTL
                Static IP: 10.0.2.15 (QEMU SLIRP default)
     ↓
Ethernet      — frame encode/decode; ethertype 0x0800 (IPv4), 0x0806 (ARP)
     ↓
NIC driver    — e1000 (x86-64) / virtio-net (AArch64)
```

### QEMU Networking

- `-netdev user,id=net0` — SLIRP usermode networking (no root required)
- Guest static IP: `10.0.2.15`, gateway: `10.0.2.2`, DNS: `10.0.2.3`
- Tests: ping `10.0.2.2`, HTTP GET via `wget` to host-side `nc -l 8080`

### Scope Limits

No IPv6, no TLS, no DHCP client, no jumbo frames, no congestion control.

---

## Userspace

### Minimal libc

Implemented independently in C and Rust. Provides:

```
Memory:   malloc / free / realloc (free-list allocator, 16-byte aligned)
String:   memcpy, memset, memmove, strlen, strcpy, strncpy, strcmp, strncmp,
          strchr, strrchr, strtok, atoi, snprintf, vsnprintf
I/O:      printf (%d %u %s %x %c %p %%), putchar, puts, getchar, fgets, fputs
File:     fopen, fclose, fread, fwrite, fseek, ftell, fflush, fileno
          stdin/stdout/stderr as FILE* backed by fd 0/1/2
Process:  exit, _exit, getpid, fork, execv, execve, waitpid, abort
Network:  socket, bind, listen, accept, connect, send, recv, sendto, recvfrom,
          close, htons, htonl, ntohs, ntohl, inet_addr, inet_ntoa
Error:    errno (thread-local), strerror, perror
```

### Dynamic Linker (`/lib/ld.so`)

```
kernel exec() loads ELF → sees PT_INTERP → maps ld.so at fixed high user VA
ld.so receives control with stack set up by kernel (argc/argv/envp/auxv)
ld.so runs before main():
  1. Read own load address from AT_BASE auxv entry
  2. Self-relocate ld.so's own GOT
  3. Read executable's PT_DYNAMIC via AT_PHDR/AT_PHNUM
  4. For each DT_NEEDED: open /lib/<name>.so, map PT_LOAD segments
  5. Resolve relocations in dependency order:
     x86-64: R_X86_64_JUMP_SLOT (PLT), R_X86_64_GLOB_DAT, R_X86_64_RELATIVE
     AArch64: R_AARCH64_JUMP_SLOT, R_AARCH64_GLOB_DAT, R_AARCH64_RELATIVE
  6. Call each library's DT_INIT and DT_INIT_ARRAY entries
  7. Call executable's DT_INIT/DT_INIT_ARRAY
  8. Jump to AT_ENTRY (program's main-wrapper / _start)
```

Supported ELF types: `ET_EXEC` (static), `ET_DYN` (PIE + shared libs).
Shared libraries are position-independent (compiled with `-fPIC` / Rust `relocation-model=pic`).

### Shell (`/bin/sh`)

```
Features:
  - Tokenizer: splits on whitespace, handles single/double quotes
  - PATH search: iterate PATH env var, stat each candidate
  - I/O redirection: >, <, >> (open + dup2 before exec)
  - Pipes: cmd1 | cmd2 → pipe(26) + fork twice + dup2 + exec both sides
  - Built-ins: cd (chdir syscall 27), exit, echo, export (env var table),
               pwd (getcwd syscall 28), unset
  - Zombie reaping: shell main loop calls waitpid(-1, WNOHANG) before each
    prompt to reap completed children. No signal delivery — polling only.
  - Prompt: displays current directory (via getcwd), reads line via fgets
```

### Included Userspace Programs

| Program | Description |
|---------|-------------|
| `ls`    | List directory (getdents syscall) |
| `cat`   | Read and print files |
| `echo`  | Print arguments |
| `mkdir` | Create directory |
| `rm`    | Remove file or directory |
| `ps`    | List processes from /proc |
| `ping`  | Send ICMP echo (raw socket, type 8/0) |
| `wget`  | HTTP/1.0 GET only; print response to stdout or file |

---

## Error Handling

**Kernel OOM:** `kmalloc` returns NULL on failure. Callers in critical paths
(scheduler, page fault handler) that cannot tolerate OOM call `panic("OOM: ...")`.
Callers that can tolerate it (VFS cache, network buffers) drop the operation and
return an error code to userspace.

**Syscall errors:** Negative return values indicate errors using a subset of
standard errno values (ENOENT, EACCES, EINVAL, ENOMEM, EBADF, EAGAIN, EPIPE,
ECONNREFUSED, ETIMEDOUT, EEXIST, ENOTDIR, EISDIR). Libc's syscall wrappers
set `errno` and return -1 on negative kernel return values.

**Kernel exceptions:** Unhandled CPU exceptions (GPF, double fault, unexpected
interrupt) call `panic()` with register dump. Page faults in kernel mode panic;
page faults in user mode deliver SIGSEGV (simplified: terminate the process).

**Driver errors:** Initialization failures (PCI device not found, virtio magic
mismatch) call `panic()` — the system cannot run without network/storage drivers
in our QEMU-only environment.

---

## Testing Strategy

Each of the 4 build targets (c/x86_64, c/aarch64, rust/x86_64, rust/aarch64)
runs a QEMU smoke test suite using `expect` scripts that read QEMU serial output:

| # | Test | Pass Condition |
|---|------|----------------|
| 1 | Boot | "QOS kernel started" on serial |
| 2 | Memory | `kmalloc` + `kfree` smoke test, no panic |
| 3 | Shell | Shell prompt appears |
| 4 | Fork/exec | `echo hello` prints "hello" |
| 5 | VFS read | `cat /proc/meminfo` shows output |
| 6 | ext2 write | `echo foo > /data/test && cat /data/test` prints "foo" |
| 7 | Network | `ping 10.0.2.2` receives reply |
| 8 | TCP | `wget http://10.0.2.2:8080/` gets response from host netcat |

Tests run via `make test-all`; each QEMU instance times out after 30 seconds.

---

## Non-Goals

- SMP / multicore (single core only)
- IPv6, TLS, DHCP client, congestion control
- POSIX compliance
- x86 32-bit or ARM 32-bit (secondary targets, not in initial scope)
- Sound, USB, GPU drivers
- Signals (no signal delivery mechanism; user-mode faults terminate the process;
  shell reaps children by polling waitpid, not via SIGCHLD delivery)
- Demand paging / swap
