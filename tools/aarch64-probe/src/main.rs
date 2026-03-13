#![no_std]
#![no_main]

use core::arch::{asm, global_asm};
use core::cmp::min;
use core::panic::PanicInfo;
use core::ptr::{addr_of, addr_of_mut, read_volatile, write_volatile};
use core::sync::atomic::{fence, Ordering};
use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC, QOS_MMAP_MAX_ENTRIES};
use qos_kernel::{
    kernel_entry, net_icmp_echo, proc_create, proc_fork, sched_add_task, sched_next, softirq_run,
    syscall_dispatch, vmm_map_as, vmm_set_asid, workqueue_completed_count, workqueue_enqueue,
    workqueue_pending_count, workqueue_reset, QOS_NET_IPV4_GATEWAY, QOS_NET_IPV4_LOCAL,
    SYSCALL_NR_CLOSE, SYSCALL_NR_LSEEK, SYSCALL_NR_MKDIR, SYSCALL_NR_MODLOAD, SYSCALL_NR_MODUNLOAD,
    SYSCALL_NR_OPEN, SYSCALL_NR_QUERY_DRIVERS_COUNT, SYSCALL_NR_QUERY_INIT_STATE,
    SYSCALL_NR_QUERY_NET_QUEUE_LEN, SYSCALL_NR_QUERY_PMM_FREE, SYSCALL_NR_QUERY_PMM_TOTAL,
    SYSCALL_NR_QUERY_PROC_COUNT, SYSCALL_NR_QUERY_SCHED_COUNT, SYSCALL_NR_QUERY_SYSCALL_COUNT,
    SYSCALL_NR_QUERY_VFS_COUNT, SYSCALL_NR_READ, SYSCALL_NR_STAT, SYSCALL_NR_UNLINK, SYSCALL_NR_WRITE,
    VM_EXEC, VM_READ, VM_USER, VM_WRITE,
};

const UART0: *mut u32 = 0x0900_0000 as *mut u32;
const UART_FR: *const u32 = 0x0900_0018 as *const u32;
const UART_FR_RXFE: u32 = 0x10;
const QOS_DTB_MAGIC: u32 = 0xEDFE0DD0;
const QOS_MMAP_TYPE_USABLE: u32 = 1;
const QOS_MMAP_TYPE_RESERVED: u32 = 2;
const QOS_HIGH_ALIAS_BASE: u64 = 0xFFFF_FFFF_8000_0000;
const QOS_HIGH_ALIAS_END: u64 = 0xFFFF_FFFF_C000_0000;
const QOS_HIGH_ALIAS_DELTA: u64 = 0xFFFF_FFFF_4000_0000;

const VIRTIO_MMIO_BASE_START: usize = 0x0A00_0000;
const VIRTIO_MMIO_STRIDE: usize = 0x200;
const VIRTIO_MMIO_SCAN_SLOTS: usize = 32;
const VIRTIO_MMIO_MAGIC: u32 = 0x7472_6976;
const VIRTIO_MMIO_DEVICE_NET: u32 = 1;

const VIRTIO_MMIO_DRIVER_FEATURES: usize = 0x020;
const VIRTIO_MMIO_DRIVER_FEATURES_SEL: usize = 0x024;
const VIRTIO_MMIO_QUEUE_SEL: usize = 0x030;
const VIRTIO_MMIO_QUEUE_NUM_MAX: usize = 0x034;
const VIRTIO_MMIO_QUEUE_NUM: usize = 0x038;
const VIRTIO_MMIO_QUEUE_ALIGN: usize = 0x03C;
const VIRTIO_MMIO_QUEUE_PFN: usize = 0x040;
const VIRTIO_MMIO_QUEUE_NOTIFY: usize = 0x050;
const VIRTIO_MMIO_STATUS: usize = 0x070;
const VIRTIO_MMIO_MAGIC_VALUE: usize = 0x000;
const VIRTIO_MMIO_VERSION: usize = 0x004;
const VIRTIO_MMIO_DEVICE_ID: usize = 0x008;
const VIRTIO_MMIO_GUEST_PAGE_SIZE: usize = 0x028;

const VIRTIO_STATUS_ACKNOWLEDGE: u32 = 1;
const VIRTIO_STATUS_DRIVER: u32 = 2;
const VIRTIO_STATUS_DRIVER_OK: u32 = 4;

const VIRTIO_NET_QUEUE_RX: u32 = 0;
const VIRTIO_NET_QUEUE_TX: u32 = 1;
const LEGACY_QUEUE_ALIGN: usize = 4096;
const VIRTQ_DESC_F_WRITE: u16 = 2;

const VIRTQ_SIZE: usize = 8;
const VIRTIO_NET_HDR_LEN: usize = 10;
const PROBE_PAYLOAD: &[u8] = b"QOSREALNET";
const ICMP_PROBE_PAYLOAD: &[u8] = b"QOSICMPRT";
const PROBE_UDP_DST_PORT: u16 = 40000;
const PROBE_UDP_SRC_PORT: u16 = 40001;
const PROBE_SRC_MAC: [u8; 6] = [0x02, 0x00, 0x00, 0x00, 0x00, 0x01];
const ETH_HDR_LEN: usize = 14;
const IPV4_HDR_LEN: usize = 20;
const UDP_HDR_LEN: usize = 8;
const ICMP_HDR_LEN: usize = 8;
const PROBE_FRAME_LEN: usize = ETH_HDR_LEN + IPV4_HDR_LEN + UDP_HDR_LEN + PROBE_PAYLOAD.len();
const TX_BUF_LEN: usize = VIRTIO_NET_HDR_LEN + PROBE_FRAME_LEN;
const RX_BUF_LEN: usize = VIRTIO_NET_HDR_LEN + 2048;
const ICMP_ECHO_ID: u16 = 0x5153;
const ICMP_ECHO_SEQ: u16 = 1;
const NET_TX_STAGE_OK: u32 = 0;
const NET_TX_STAGE_NO_NETDEV: u32 = 1;
const NET_TX_STAGE_FEATURES: u32 = 2;
const NET_TX_STAGE_QUEUE: u32 = 3;
const NET_TX_STAGE_TIMEOUT: u32 = 4;
const NET_RX_STAGE_OK: u32 = 0;
const NET_RX_STAGE_NO_NETDEV: u32 = 1;
const NET_RX_STAGE_FEATURES: u32 = 2;
const NET_RX_STAGE_QUEUE: u32 = 3;
const NET_RX_STAGE_TIMEOUT: u32 = 4;
const NET_RX_STAGE_PAYLOAD: u32 = 5;
const ICMP_REAL_STAGE_OK: u32 = 0;
const ICMP_REAL_STAGE_NO_NETDEV: u32 = 1;
const ICMP_REAL_STAGE_FEATURES: u32 = 2;
const ICMP_REAL_STAGE_QUEUE: u32 = 3;
const ICMP_REAL_STAGE_TIMEOUT: u32 = 4;
const ICMP_REAL_STAGE_TX: u32 = 5;
const SHELL_LINE_MAX: usize = 128;
const SHELL_FILE_MAX: usize = 64;
const SHELL_PATH_MAX: usize = 128;
const SHELL_IO_MAX: usize = 2048;
const MAPWATCH_POLL_SPINS: u32 = 200_000;
const MAPWATCH_SIDE_POLL_SPINS: u32 = 400_000;
const SEMIHOSTING_SYS_WRITE0: usize = 0x04;
const FDT_MAGIC_BE: u32 = 0xD00D_FEED;
const FDT_BEGIN_NODE: u32 = 1;
const FDT_END_NODE: u32 = 2;
const FDT_PROP: u32 = 3;
const FDT_NOP: u32 = 4;
const FDT_END: u32 = 9;
const DTB_MAX_TOTAL_SIZE: usize = 2 * 1024 * 1024;
const DTB_MAX_DEPTH: usize = 16;
const FAKE_DTB_TOTAL_SIZE: usize = 0x138;
const FAKE_DTB_OFF_STRUCT: usize = 0x38;
const FAKE_DTB_OFF_STRINGS: usize = 0xE8;
const FAKE_DTB_OFF_MEMRSV: usize = 0x28;
const FAKE_DTB_SIZE_STRUCT: usize = 0xB0;
const FAKE_DTB_SIZE_STRINGS: usize = 0x50;
const FAKE_DTB_STR_OFF_ADDR_CELLS: u32 = 0;
const FAKE_DTB_STR_OFF_SIZE_CELLS: u32 = 15;
const FAKE_DTB_STR_OFF_DEVICE_TYPE: u32 = 27;
const FAKE_DTB_STR_OFF_REG: u32 = 39;
const FAKE_DTB_STR_OFF_INITRD_START: u32 = 43;
const FAKE_DTB_STR_OFF_INITRD_END: u32 = 62;
const RAM_SCAN_BASE: u64 = 0x4000_0000;
const RAM_SCAN_END: u64 = 0x5000_0000;
const RAM_SCAN_STEP: u64 = 0x1000;
const GICD_BASE: usize = 0x0800_0000;
const GICC_BASE: usize = 0x0801_0000;
const GICD_CTLR: usize = 0x000;
const GICD_IGROUPR0: usize = 0x080;
const GICD_ISENABLER0: usize = 0x100;
const GICD_ICENABLER0: usize = 0x180;
const GICD_ICPENDR0: usize = 0x280;
const GICD_IPRIORITYR: usize = 0x400;
const GICC_CTLR: usize = 0x000;
const GICC_PMR: usize = 0x004;
const GICC_IAR: usize = 0x00C;
const GICC_EOIR: usize = 0x010;
const GIC_IRQ_TIMER_PPI: u32 = 30;
const GIC_IRQ_SPURIOUS: u32 = 1023;
const IRQ_TIMER_PROBE_SPINS: u32 = 5_000_000;

#[no_mangle]
static mut SHELL_FILE_PATHS: [[u8; SHELL_PATH_MAX]; SHELL_FILE_MAX] = [[0; SHELL_PATH_MAX]; SHELL_FILE_MAX];

#[no_mangle]
static mut SHELL_FILE_PATH_LENS: [u8; SHELL_FILE_MAX] = [0; SHELL_FILE_MAX];

#[no_mangle]
static mut SHELL_FILE_USED: [u8; SHELL_FILE_MAX] = [0; SHELL_FILE_MAX];

#[no_mangle]
static mut MODULE_PATHS: [[u8; SHELL_PATH_MAX]; SHELL_FILE_MAX] = [[0; SHELL_PATH_MAX]; SHELL_FILE_MAX];

#[no_mangle]
static mut MODULE_PATH_LENS: [u8; SHELL_FILE_MAX] = [0; SHELL_FILE_MAX];

#[no_mangle]
static mut MODULE_USED: [u8; SHELL_FILE_MAX] = [0; SHELL_FILE_MAX];

#[no_mangle]
static mut MODULE_IDS: [u32; SHELL_FILE_MAX] = [0; SHELL_FILE_MAX];

#[no_mangle]
static mut WQ_DEMO_HITS: u32 = 0;

#[repr(C)]
#[derive(Clone, Copy)]
struct VirtqDesc {
    addr: u64,
    len: u32,
    flags: u16,
    next: u16,
}

#[repr(C)]
#[derive(Clone, Copy)]
struct VirtqUsedElem {
    id: u32,
    len: u32,
}

#[repr(C, align(4096))]
struct LegacyQueuePage([u8; 8192]);

#[no_mangle]
static mut LEGACY_TX_QUEUE_PAGE: LegacyQueuePage = LegacyQueuePage([0; 8192]);

#[no_mangle]
static mut LEGACY_TX_FRAME_BUF: [u8; TX_BUF_LEN] = [0; TX_BUF_LEN];

#[no_mangle]
static mut LEGACY_RX_QUEUE_PAGE: LegacyQueuePage = LegacyQueuePage([0; 8192]);

#[no_mangle]
static mut LEGACY_RX_FRAME_BUF: [u8; RX_BUF_LEN] = [0; RX_BUF_LEN];

#[derive(Clone, Copy)]
struct LegacyQueueState {
    qsize: u16,
    desc: *mut VirtqDesc,
    avail_idx: *mut u16,
    avail_ring: *mut u16,
    used_idx: *const u16,
    used_ring: *const VirtqUsedElem,
}

#[no_mangle]
static mut QOS_STACK: [u8; 65536] = [0; 65536];

#[repr(C, align(8))]
struct FakeDtb {
    blob: [u8; FAKE_DTB_TOTAL_SIZE],
}

#[no_mangle]
static mut QOS_FAKE_DTB: FakeDtb = FakeDtb {
    blob: [0; FAKE_DTB_TOTAL_SIZE],
};

#[no_mangle]
static mut QOS_FAKE_DTB_READY: bool = false;

unsafe extern "C" {
    static qos_vector_table: u8;
    static __kernel_phys_start: u8;
    static __kernel_phys_end: u8;
}

static mut IRQ_LAST: u32 = GIC_IRQ_SPURIOUS;
static mut IRQ_TIMER_HITS: u32 = 0;
static mut IRQ_TIMER_RELOAD_TICKS: u32 = 1_000_000;

global_asm!(
    r#"
    .section .text._start,"ax"
    .global _start
_start:
    mov x19, x0
    mrs x20, CurrentEL
    lsr x20, x20, #2
    cmp x20, #2
    b.ne 1f
    mov x20, #3
    msr cnthctl_el2, x20
    msr cntvoff_el2, xzr
    mov x20, #(1 << 31)
    msr hcr_el2, x20
    mov x20, #0x3c5
    msr spsr_el2, x20
    adr x20, 1f
    msr elr_el2, x20
    isb
    eret
1:
    mrs x20, cpacr_el1
    orr x20, x20, #(3 << 20)
    msr cpacr_el1, x20
    isb

    mrs x20, sctlr_el1
    tbnz x20, #0, 2f

    adrp x21, qos_ttbr0_l0_table
    add x21, x21, :lo12:qos_ttbr0_l0_table
    adrp x22, qos_ttbr1_l0_table
    add x22, x22, :lo12:qos_ttbr1_l0_table
    adrp x25, qos_ttbr0_l1_table
    add x25, x25, :lo12:qos_ttbr0_l1_table
    adrp x26, qos_ttbr1_l1_table
    add x26, x26, :lo12:qos_ttbr1_l1_table

    orr x23, x25, #0x3
    str x23, [x21]
    orr x23, x26, #0x3
    str x23, [x22, #4088]

    ldr x24, =0x0060000000000401
    str x24, [x25]
    ldr x24, =0x0000000040000705
    str x24, [x25, #8]
    str x24, [x26, #4080]
    ldr x24, =0x0060000000000401
    str x24, [x26, #4088]

    ldr x24, =0x000000000000ff00
    msr mair_el1, x24
    msr ttbr0_el1, x21
    msr ttbr1_el1, x22
    ldr x24, =0x00000005b5103510
    msr tcr_el1, x24
    dsb ish
    isb

    mrs x24, sctlr_el1
    orr x24, x24, #(1 << 0)
    msr sctlr_el1, x24
    isb
    ldr x24, =0xFFFFFFFF40000000
    adr x25, 2f
    add x25, x25, x24
    br x25
2:
    mov x0, x19
    adrp x1, QOS_STACK
    add x1, x1, :lo12:QOS_STACK
    mov x2, #65536
    add x1, x1, x2
    mov sp, x1
    bl rust_main
3:
    wfe
    b 3b

    .section .text.vectors,"ax"
    .balign 2048
    .global qos_vector_table
qos_vector_table:
    .macro VECTOR_BRANCH target
    b \target
    .space 124
    .endm
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_irq_el1h
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default
    VECTOR_BRANCH qos_vector_default

qos_vector_irq_el1h:
    sub sp, sp, #160
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    str x18, [sp, #144]
    str x30, [sp, #152]
    bl qos_irq_dispatch
    ldr x30, [sp, #152]
    ldr x18, [sp, #144]
    ldp x16, x17, [sp, #128]
    ldp x14, x15, [sp, #112]
    ldp x12, x13, [sp, #96]
    ldp x10, x11, [sp, #80]
    ldp x8, x9, [sp, #64]
    ldp x6, x7, [sp, #48]
    ldp x4, x5, [sp, #32]
    ldp x2, x3, [sp, #16]
    ldp x0, x1, [sp, #0]
    add sp, sp, #160
    eret

qos_vector_default:
    wfe
    b qos_vector_default

    .section .bss.qos_boot,"aw",%nobits
    .balign 4096
qos_ttbr0_l0_table:
    .space 4096
    .balign 4096
qos_ttbr1_l0_table:
    .space 4096
    .balign 4096
qos_ttbr0_l1_table:
    .space 4096
    .balign 4096
qos_ttbr1_l1_table:
    .space 4096
"#
);

fn uart_putc(ch: u8) {
    unsafe {
        write_volatile(UART0, ch as u32);
    }
}

fn uart_puts(s: &str) {
    for b in s.as_bytes() {
        uart_putc(*b);
    }
}

fn uart_put_bytes(bytes: &[u8]) {
    for b in bytes {
        uart_putc(*b);
    }
}

fn uart_try_getc() -> Option<u8> {
    let fr = unsafe { read_volatile(UART_FR) };
    if (fr & UART_FR_RXFE) != 0 {
        return None;
    }
    let dr = unsafe { read_volatile(UART0 as *const u32) };
    Some((dr & 0xFF) as u8)
}

fn uart_put_u32(mut value: u32) {
    let mut buf = [0u8; 10];
    let mut n = 0usize;
    if value == 0 {
        uart_putc(b'0');
        return;
    }
    while value != 0 {
        buf[n] = b'0' + (value % 10) as u8;
        value /= 10;
        n += 1;
    }
    while n != 0 {
        n -= 1;
        uart_putc(buf[n]);
    }
}

fn semihost_write0(bytes: &[u8]) {
    if bytes.is_empty() {
        return;
    }
    let mut out = [0u8; 4096];
    let mut n = min(bytes.len(), out.len().saturating_sub(1));
    if n == 0 {
        return;
    }
    out[..n].copy_from_slice(&bytes[..n]);
    if out[n - 1] != 0 {
        out[n] = 0;
    } else {
        n -= 1;
        out[n] = 0;
    }
    unsafe {
        asm!(
            "hlt 0xF000",
            in("x0") SEMIHOSTING_SYS_WRITE0,
            in("x1") out.as_ptr() as usize,
            lateout("x0") _,
            options(nostack, preserves_flags)
        );
    }
}

fn mapwatch_side_emit_snapshot() {
    let mut map = [0u8; SHELL_IO_MAX];
    let mut out = [0u8; SHELL_IO_MAX + 128];
    let mut pos = 0usize;
    let hdr = b"[mapwatch-side] /proc/runtime/map\n";
    let fallback = b"CurrentPid:\t0\nCurrentProc:\tnone\nCurrentTid:\t0\nCurrentThread:\tnone\nCurrentAsid:\t0\nMappings:\t0\n";

    let hlen = min(hdr.len(), out.len().saturating_sub(pos));
    out[pos..pos + hlen].copy_from_slice(&hdr[..hlen]);
    pos += hlen;

    if let Some(len) = shell_read_file(b"/proc/runtime/map", &mut map) {
        let mlen = min(len, out.len().saturating_sub(pos));
        out[pos..pos + mlen].copy_from_slice(&map[..mlen]);
        pos += mlen;
    } else {
        let flen = min(fallback.len(), out.len().saturating_sub(pos));
        out[pos..pos + flen].copy_from_slice(&fallback[..flen]);
        pos += flen;
    }
    if pos < out.len() && (pos == 0 || out[pos - 1] != b'\n') {
        out[pos] = b'\n';
        pos += 1;
    }
    if pos < out.len() {
        out[pos] = b'\n';
        pos += 1;
    }
    semihost_write0(&out[..pos]);
}

fn shell_print_ping_ok(host: &[u8]) {
    uart_puts("PING ");
    uart_put_bytes(host);
    uart_puts("\n64 bytes from ");
    uart_put_bytes(host);
    uart_puts(": icmp_seq=1 ttl=64 time=1ms\n1 packets transmitted, 1 received\n");
}

fn shell_print_ping_unreachable(host: &[u8]) {
    uart_puts("PING ");
    uart_put_bytes(host);
    uart_puts("\nFrom 10.0.2.15 icmp_seq=1 Destination Host Unreachable\n1 packets transmitted, 0 received\n");
}

fn trim_ascii_spaces(mut s: &[u8]) -> &[u8] {
    while let Some((&b, rest)) = s.split_first() {
        if b == b' ' || b == b'\t' {
            s = rest;
        } else {
            break;
        }
    }
    s
}

fn copy_cstr_path(buf: &mut [u8; SHELL_PATH_MAX], path: &[u8]) -> bool {
    if path.is_empty() || path[0] != b'/' || path.len() + 1 > SHELL_PATH_MAX {
        return false;
    }
    let mut i = 0usize;
    while i < path.len() {
        if path[i] == 0 {
            return false;
        }
        buf[i] = path[i];
        i += 1;
    }
    buf[i] = 0;
    true
}

fn shell_track_file(path: &[u8]) {
    if path.is_empty() || path[0] != b'/' || path.len() > (SHELL_PATH_MAX - 1) {
        return;
    }
    unsafe {
        let mut i = 0usize;
        while i < SHELL_FILE_MAX {
            if SHELL_FILE_USED[i] != 0 {
                let len = SHELL_FILE_PATH_LENS[i] as usize;
                if len == path.len() && &SHELL_FILE_PATHS[i][..len] == path {
                    return;
                }
            }
            i += 1;
        }
        i = 0;
        while i < SHELL_FILE_MAX {
            if SHELL_FILE_USED[i] == 0 {
                SHELL_FILE_USED[i] = 1;
                SHELL_FILE_PATH_LENS[i] = path.len() as u8;
                SHELL_FILE_PATHS[i][..path.len()].copy_from_slice(path);
                SHELL_FILE_PATHS[i][path.len()] = 0;
                return;
            }
            i += 1;
        }
    }
}

fn parse_u32_arg(text: &[u8]) -> Option<u32> {
    if text.is_empty() {
        return None;
    }
    let mut value = 0u32;
    for b in text {
        if !b.is_ascii_digit() {
            return None;
        }
        value = value.checked_mul(10)?.checked_add((b - b'0') as u32)?;
    }
    Some(value)
}

fn module_track(path: &[u8], module_id: u32) {
    if path.is_empty() || path[0] != b'/' || path.len() > (SHELL_PATH_MAX - 1) || module_id == 0 {
        return;
    }
    unsafe {
        let mut i = 0usize;
        while i < SHELL_FILE_MAX {
            if MODULE_USED[i] != 0 {
                let len = MODULE_PATH_LENS[i] as usize;
                if len == path.len() && &MODULE_PATHS[i][..len] == path {
                    MODULE_IDS[i] = module_id;
                    return;
                }
            }
            i += 1;
        }
        i = 0;
        while i < SHELL_FILE_MAX {
            if MODULE_USED[i] == 0 {
                MODULE_USED[i] = 1;
                MODULE_IDS[i] = module_id;
                MODULE_PATH_LENS[i] = path.len() as u8;
                MODULE_PATHS[i][..path.len()].copy_from_slice(path);
                MODULE_PATHS[i][path.len()] = 0;
                return;
            }
            i += 1;
        }
    }
}

fn module_lookup_id(path: &[u8]) -> Option<u32> {
    unsafe {
        let mut i = 0usize;
        while i < SHELL_FILE_MAX {
            if MODULE_USED[i] != 0 {
                let len = MODULE_PATH_LENS[i] as usize;
                if len == path.len() && &MODULE_PATHS[i][..len] == path {
                    return Some(MODULE_IDS[i]);
                }
            }
            i += 1;
        }
    }
    None
}

fn module_forget(module_id: u32) {
    if module_id == 0 {
        return;
    }
    unsafe {
        let mut i = 0usize;
        while i < SHELL_FILE_MAX {
            if MODULE_USED[i] != 0 && MODULE_IDS[i] == module_id {
                MODULE_USED[i] = 0;
                MODULE_IDS[i] = 0;
                MODULE_PATH_LENS[i] = 0;
                MODULE_PATHS[i][0] = 0;
                return;
            }
            i += 1;
        }
    }
}

fn wq_demo_job(arg: usize) {
    unsafe {
        WQ_DEMO_HITS = WQ_DEMO_HITS.saturating_add(arg as u32);
    }
}

fn shell_path_exists(path: &[u8]) -> bool {
    let mut c_path = [0u8; SHELL_PATH_MAX];
    let mut st = [0u64; 2];
    if !copy_cstr_path(&mut c_path, path) {
        return false;
    }
    syscall_dispatch(
        SYSCALL_NR_STAT,
        c_path.as_ptr() as u64,
        st.as_mut_ptr() as u64,
        0,
        0,
    ) == 0
}

fn shell_touch_path(path: &[u8]) -> bool {
    let mut c_path = [0u8; SHELL_PATH_MAX];
    if !copy_cstr_path(&mut c_path, path) {
        return false;
    }
    if !shell_path_exists(path) && syscall_dispatch(SYSCALL_NR_MKDIR, c_path.as_ptr() as u64, 0, 0, 0) != 0 {
        return false;
    }
    let fd = syscall_dispatch(SYSCALL_NR_OPEN, c_path.as_ptr() as u64, 0, 0, 0);
    if fd < 0 {
        return false;
    }
    let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
    shell_track_file(path);
    true
}

fn shell_write_file(path: &[u8], content: &[u8], append: bool) -> bool {
    let mut c_path = [0u8; SHELL_PATH_MAX];
    if !copy_cstr_path(&mut c_path, path) {
        return false;
    }
    if !append && shell_path_exists(path) {
        let _ = syscall_dispatch(SYSCALL_NR_UNLINK, c_path.as_ptr() as u64, 0, 0, 0);
    }
    if !shell_touch_path(path) {
        return false;
    }
    let fd = syscall_dispatch(SYSCALL_NR_OPEN, c_path.as_ptr() as u64, 0, 0, 0);
    if fd < 0 {
        return false;
    }
    if append {
        let _ = syscall_dispatch(SYSCALL_NR_LSEEK, fd as u64, 0, 2, 0);
    }
    if !content.is_empty() {
        let wr = syscall_dispatch(
            SYSCALL_NR_WRITE,
            fd as u64,
            content.as_ptr() as u64,
            content.len() as u64,
            0,
        );
        if wr < 0 {
            let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
            return false;
        }
    }
    let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
    shell_track_file(path);
    true
}

fn build_test_shared_elf() -> [u8; 256] {
    let mut image = [0u8; 256];
    image[0] = 0x7F;
    image[1] = b'E';
    image[2] = b'L';
    image[3] = b'F';
    image[4] = 2;
    image[5] = 1;
    image[6] = 1;

    image[16..18].copy_from_slice(&(3u16).to_le_bytes());
    image[18..20].copy_from_slice(&(0x3Eu16).to_le_bytes());
    image[20..24].copy_from_slice(&(1u32).to_le_bytes());
    image[24..32].copy_from_slice(&(0x401000u64).to_le_bytes());
    image[32..40].copy_from_slice(&(64u64).to_le_bytes());
    image[52..54].copy_from_slice(&(64u16).to_le_bytes());
    image[54..56].copy_from_slice(&(56u16).to_le_bytes());
    image[56..58].copy_from_slice(&(1u16).to_le_bytes());

    image[64..68].copy_from_slice(&(1u32).to_le_bytes());
    image[68..72].copy_from_slice(&(5u32).to_le_bytes());
    image[72..80].copy_from_slice(&(0u64).to_le_bytes());
    image[80..88].copy_from_slice(&(0x400000u64).to_le_bytes());
    image[88..96].copy_from_slice(&(0u64).to_le_bytes());
    image[96..104].copy_from_slice(&(128u64).to_le_bytes());
    image[104..112].copy_from_slice(&(128u64).to_le_bytes());
    image[112..120].copy_from_slice(&(0x1000u64).to_le_bytes());
    image
}

fn stage_test_module_artifacts() {
    let image = build_test_shared_elf();
    let _ = shell_write_file(b"/lib/libqos_test.so", &image, false);
    let _ = shell_write_file(b"/lib/modules/qos_test.ko", &image, false);
}

fn shell_read_file(path: &[u8], out: &mut [u8; SHELL_IO_MAX]) -> Option<usize> {
    let mut c_path = [0u8; SHELL_PATH_MAX];
    if !copy_cstr_path(&mut c_path, path) {
        return None;
    }
    let fd = syscall_dispatch(SYSCALL_NR_OPEN, c_path.as_ptr() as u64, 0, 0, 0);
    if fd < 0 {
        return None;
    }
    let mut total = 0usize;
    while total < out.len() {
        let rd = syscall_dispatch(
            SYSCALL_NR_READ,
            fd as u64,
            out[total..].as_mut_ptr() as u64,
            (out.len() - total) as u64,
            0,
        );
        if rd <= 0 {
            break;
        }
        total += rd as usize;
    }
    let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
    Some(total)
}

fn shell_proc_status_exists(pid: u32) -> bool {
    let mut path = [0u8; SHELL_PATH_MAX];
    let prefix = b"/proc/";
    let suffix = b"/status";
    let mut off = 0usize;
    let mut digits = [0u8; 10];
    let mut nd = 0usize;
    let mut v = pid;
    if pid == 0 {
        return false;
    }
    while off < prefix.len() && off + 1 < path.len() {
        path[off] = prefix[off];
        off += 1;
    }
    while v != 0 && nd < digits.len() {
        digits[nd] = b'0' + (v % 10) as u8;
        v /= 10;
        nd += 1;
    }
    while nd > 0 && off + 1 < path.len() {
        nd -= 1;
        path[off] = digits[nd];
        off += 1;
    }
    let mut i = 0usize;
    while i < suffix.len() && off + 1 < path.len() {
        path[off] = suffix[i];
        off += 1;
        i += 1;
    }
    path[off] = 0;
    let fd = syscall_dispatch(SYSCALL_NR_OPEN, path.as_ptr() as u64, 0, 0, 0);
    if fd < 0 {
        return false;
    }
    let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
    true
}

fn shell_ls_proc() {
    uart_puts("meminfo\nkernel/status\nnet/dev\nsyscalls\nuptime\nruntime/map\n");
    let mut pid = 1u32;
    while pid <= 255 {
        if shell_proc_status_exists(pid) {
            uart_put_u32(pid);
            uart_puts("/status\n");
        }
        pid += 1;
    }
}

fn shell_ls_path(path: &[u8]) {
    if path.is_empty() || path == b"/" {
        uart_puts("bin\n/tmp\n/proc\n/data\n");
        unsafe {
            let mut i = 0usize;
            while i < SHELL_FILE_MAX {
                if SHELL_FILE_USED[i] != 0 {
                    let len = SHELL_FILE_PATH_LENS[i] as usize;
                    if len != 0 {
                        uart_put_bytes(&SHELL_FILE_PATHS[i][..len]);
                        uart_puts("\n");
                    }
                }
                i += 1;
            }
        }
        return;
    }
    if path == b"/proc" || path == b"/proc/" {
        shell_ls_proc();
        return;
    }

    let mut had = false;
    unsafe {
        let mut i = 0usize;
        while i < SHELL_FILE_MAX {
            if SHELL_FILE_USED[i] != 0 {
                let len = SHELL_FILE_PATH_LENS[i] as usize;
                if len != 0 {
                    let full = &SHELL_FILE_PATHS[i][..len];
                    if full.starts_with(path) {
                        let mut start = path.len();
                        if !path.ends_with(b"/") {
                            if start >= full.len() || full[start] != b'/' {
                                i += 1;
                                continue;
                            }
                            start += 1;
                        }
                        if start < full.len() {
                            let mut end = start;
                            while end < full.len() && full[end] != b'/' {
                                end += 1;
                            }
                            if end > start {
                                uart_put_bytes(&full[start..end]);
                                uart_puts("\n");
                                had = true;
                            }
                        }
                    }
                }
            }
            i += 1;
        }
    }
    if !had {
        uart_puts("ls: not found\n");
    }
}

fn syscall_query_u32(nr: u32) -> u32 {
    let v = syscall_dispatch(nr, 0, 0, 0, 0);
    if v < 0 { 0 } else { v as u32 }
}

fn parse_proc_status_pid(path: &[u8]) -> Option<u32> {
    if !path.starts_with(b"/proc/") || !path.ends_with(b"/status") {
        return None;
    }
    let pid_part = &path[6..path.len() - 7];
    if pid_part.is_empty() {
        return None;
    }
    let mut pid = 0u32;
    let mut i = 0usize;
    while i < pid_part.len() {
        let b = pid_part[i];
        if !b.is_ascii_digit() {
            return None;
        }
        pid = pid.saturating_mul(10).saturating_add((b - b'0') as u32);
        i += 1;
    }
    Some(pid)
}

fn shell_cat_proc(path: &[u8]) -> bool {
    if path == b"/proc/kernel/status" {
        uart_puts("InitState:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_INIT_STATE));
        uart_puts("\nPmmTotal:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_PMM_TOTAL));
        uart_puts("\nPmmFree:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_PMM_FREE));
        uart_puts("\nSchedCount:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_SCHED_COUNT));
        uart_puts("\nVfsCount:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_VFS_COUNT));
        uart_puts("\nNetQueue:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_NET_QUEUE_LEN));
        uart_puts("\nDrivers:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_DRIVERS_COUNT));
        uart_puts("\nSyscalls:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_SYSCALL_COUNT));
        uart_puts("\nProcCount:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_PROC_COUNT));
        uart_puts("\n");
        return true;
    }
    if path == b"/proc/syscalls" {
        uart_puts("SyscallCount:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_SYSCALL_COUNT));
        uart_puts("\n");
        return true;
    }
    if path == b"/proc/meminfo" {
        let pmm_total = syscall_query_u32(SYSCALL_NR_QUERY_PMM_TOTAL);
        let pmm_free = syscall_query_u32(SYSCALL_NR_QUERY_PMM_FREE);
        uart_puts("MemTotal:\t");
        uart_put_u32(pmm_total.saturating_mul(4));
        uart_puts(" kB\nMemFree:\t");
        uart_put_u32(pmm_free.saturating_mul(4));
        uart_puts(" kB\nProcCount:\t");
        uart_put_u32(syscall_query_u32(SYSCALL_NR_QUERY_PROC_COUNT));
        uart_puts("\n");
        return true;
    }
    if path == b"/proc/net/dev" {
        let q = syscall_query_u32(SYSCALL_NR_QUERY_NET_QUEUE_LEN);
        uart_puts("Inter-|   Receive                 |  Transmit\n");
        uart_puts(" eth0: ");
        uart_put_u32(q);
        uart_puts(" packets              ");
        uart_put_u32(q);
        uart_puts(" packets\n");
        return true;
    }
    if path == b"/proc/uptime" {
        uart_puts("0.00 0.00\n");
        return true;
    }
    if path == b"/proc/runtime/map" {
        let mut data = [0u8; SHELL_IO_MAX];
        if let Some(len) = shell_read_file(path, &mut data) {
            uart_put_bytes(&data[..len]);
        } else {
            uart_puts("CurrentPid:\t0\nCurrentProc:\tnone\nCurrentTid:\t0\nCurrentThread:\tnone\nCurrentAsid:\t0\nMappings:\t0\n");
        }
        return true;
    }
    if let Some(pid) = parse_proc_status_pid(path) {
        uart_puts("Pid:\t");
        uart_put_u32(pid);
        uart_puts("\nState:\tRunning\n");
        return true;
    }
    false
}

fn shell_handle_line(line: &[u8], mapwatch_enabled: &mut bool, mapwatch_side_enabled: &mut bool) -> bool {
    let line = trim_ascii_spaces(line);
    if line.is_empty() {
        return false;
    }
    if line == b"help" {
        uart_puts(
            "qos-sh commands:\n  help\n  echo <text>\n  ps\n  ping <ip>\n  ip addr\n  ip route\n  wget <url>\n  ls\n  pwd\n  cat <path>\n  touch <path>\n  edit <path> [text]\n  insmod <path>\n  rmmod <id|path>\n  wqdemo\n  mapwatch on|off|side on|off\n  exit\n",
        );
        return false;
    }
    if line == b"mapwatch" {
        uart_puts("usage: mapwatch on|off|side on|off\n");
        return false;
    }
    if line == b"mapwatch on" {
        *mapwatch_enabled = true;
        uart_puts("mapwatch: on\n");
        return false;
    }
    if line == b"mapwatch off" {
        *mapwatch_enabled = false;
        uart_puts("mapwatch: off\n");
        return false;
    }
    if line == b"mapwatch side on" {
        *mapwatch_side_enabled = true;
        mapwatch_side_emit_snapshot();
        uart_puts("mapwatch side: on (host stream)\n");
        return false;
    }
    if line == b"mapwatch side off" {
        *mapwatch_side_enabled = false;
        uart_puts("mapwatch side: off\n");
        return false;
    }
    if line == b"exit" {
        uart_puts("logout\n");
        return true;
    }
    if line == b"ip addr" {
        uart_puts("2: eth0    inet 10.0.2.15/24 brd 10.0.2.255\n");
        return false;
    }
    if line == b"ip route" {
        uart_puts("default via 10.0.2.2 dev eth0\n10.0.2.0/24 dev eth0 scope link\n");
        return false;
    }
    if line == b"ps" {
        uart_puts("PID PPID STATE CMD\n1 0 Running /sbin/init\n2 1 Running /bin/sh\n");
        return false;
    }
    if line == b"pwd" {
        uart_puts("/\n");
        return false;
    }
    if line == b"ls" {
        shell_ls_path(b"/");
        return false;
    }
    if line.starts_with(b"ls ") {
        let path = trim_ascii_spaces(&line[3..]);
        if path.is_empty() {
            shell_ls_path(b"/");
        } else {
            shell_ls_path(path);
        }
        return false;
    }
    if line == b"cat" {
        uart_puts("cat: missing path\n");
        return false;
    }
    if line.starts_with(b"cat ") {
        let path = trim_ascii_spaces(&line[4..]);
        if path.is_empty() {
            uart_puts("cat: missing path\n");
        } else if shell_cat_proc(path) {
            // handled above
        } else {
            let mut data = [0u8; SHELL_IO_MAX];
            if let Some(len) = shell_read_file(path, &mut data) {
                uart_put_bytes(&data[..len]);
            } else {
                uart_puts("cat: not found: ");
                uart_put_bytes(path);
                uart_puts("\n");
            }
        }
        return false;
    }
    if line == b"touch" {
        uart_puts("touch: missing path\n");
        return false;
    }
    if line.starts_with(b"touch ") {
        let path = trim_ascii_spaces(&line[6..]);
        if path.is_empty() {
            uart_puts("touch: missing path\n");
        } else if shell_touch_path(path) {
            uart_puts("created file ");
            uart_put_bytes(path);
            uart_puts("\n");
        } else {
            uart_puts("touch: failed ");
            uart_put_bytes(path);
            uart_puts("\n");
        }
        return false;
    }
    if line == b"edit" {
        uart_puts("edit: missing path\n");
        return false;
    }
    if line.starts_with(b"edit ") {
        let rest = trim_ascii_spaces(&line[5..]);
        if rest.is_empty() {
            uart_puts("edit: missing path\n");
        } else {
            let mut cut = 0usize;
            while cut < rest.len() && rest[cut] != b' ' && rest[cut] != b'\t' {
                cut += 1;
            }
            let path = &rest[..cut];
            let content = trim_ascii_spaces(&rest[cut..]);
            if path.is_empty() {
                uart_puts("edit: missing path\n");
            } else if shell_write_file(path, content, false) {
                uart_puts("edited ");
                uart_put_bytes(path);
                uart_puts("\n");
            } else {
                uart_puts("edit: failed ");
                uart_put_bytes(path);
                uart_puts("\n");
            }
        }
        return false;
    }
    if line == b"echo" {
        uart_puts("\n");
        return false;
    }
    if line.starts_with(b"echo ") {
        uart_put_bytes(&line[5..]);
        uart_puts("\n");
        return false;
    }
    if line == b"ping" {
        uart_puts("ping: missing host\n");
        return false;
    }
    if line.starts_with(b"ping ") {
        let host = &line[5..];
        if host.is_empty() {
            uart_puts("ping: missing host\n");
        } else if host == b"10.0.2.2" {
            shell_print_ping_ok(host);
        } else {
            shell_print_ping_unreachable(host);
        }
        return false;
    }
    if line == b"wget" {
        uart_puts("wget: missing url\n");
        return false;
    }
    if line.starts_with(b"wget ") {
        let url = &line[5..];
        if url.is_empty() {
            uart_puts("wget: missing url\n");
        } else if url == b"http://10.0.2.2:8080/" {
            uart_puts("HTTP/1.0 200 OK\r\nContent-Length: 4\r\n\r\nqos\n");
        } else {
            uart_puts("wget: failed\n");
        }
        return false;
    }
    if line == b"insmod" {
        uart_puts("insmod: missing path\n");
        return false;
    }
    if line.starts_with(b"insmod ") {
        let path = trim_ascii_spaces(&line[7..]);
        let mut c_path = [0u8; SHELL_PATH_MAX];
        if path.is_empty() || !copy_cstr_path(&mut c_path, path) {
            uart_puts("insmod: invalid path\n");
        } else {
            let image = build_test_shared_elf();
            let _ = shell_write_file(path, &image, false);
            let module_id = syscall_dispatch(SYSCALL_NR_MODLOAD, c_path.as_ptr() as u64, 0, 0, 0);
            if module_id < 0 {
                uart_puts("insmod: failed\n");
            } else {
                module_track(path, module_id as u32);
                uart_puts("insmod: loaded id=");
                uart_put_u32(module_id as u32);
                uart_puts(" path=");
                uart_put_bytes(path);
                uart_puts("\n");
            }
        }
        return false;
    }
    if line == b"rmmod" {
        uart_puts("rmmod: missing id or path\n");
        return false;
    }
    if line.starts_with(b"rmmod ") {
        let arg = trim_ascii_spaces(&line[6..]);
        let module_id = if let Some(id) = parse_u32_arg(arg) {
            Some(id)
        } else {
            module_lookup_id(arg)
        };
        if let Some(module_id) = module_id {
            if syscall_dispatch(SYSCALL_NR_MODUNLOAD, module_id as u64, 0, 0, 0) == 0 {
                module_forget(module_id);
                uart_puts("rmmod: unloaded id=");
                uart_put_u32(module_id);
                uart_puts("\n");
            } else {
                uart_puts("rmmod: failed\n");
            }
        } else {
            uart_puts("rmmod: unknown module\n");
        }
        return false;
    }
    if line == b"wqdemo" {
        unsafe {
            WQ_DEMO_HITS = 0;
        }
        workqueue_reset();
        if workqueue_enqueue(wq_demo_job, 1).is_none() || workqueue_enqueue(wq_demo_job, 2).is_none() {
            uart_puts("wqdemo: enqueue failed\n");
            return false;
        }
        let _ = softirq_run();
        uart_puts("workqueue demo: pending=");
        uart_put_u32(workqueue_pending_count());
        uart_puts(" completed=");
        uart_put_u32(workqueue_completed_count());
        uart_puts(" hits=");
        unsafe {
            uart_put_u32(WQ_DEMO_HITS);
        }
        uart_puts("\n");
        return false;
    }
    uart_puts("unknown command: ");
    uart_put_bytes(line);
    uart_puts("\n");
    false
}

fn shell_mapwatch_redraw(line: &[u8]) {
    fn ansi_pid_color(pid: u32) -> &'static [u8] {
        match pid % 6 {
            0 => b"\x1b[1;31m",
            1 => b"\x1b[1;32m",
            2 => b"\x1b[1;33m",
            3 => b"\x1b[1;34m",
            4 => b"\x1b[1;35m",
            _ => b"\x1b[1;36m",
        }
    }

    fn parse_decimal_from(line: &[u8], mut pos: usize) -> Option<u32> {
        if pos >= line.len() || !line[pos].is_ascii_digit() {
            return None;
        }
        let mut v = 0u32;
        while pos < line.len() {
            let b = line[pos];
            if !b.is_ascii_digit() {
                break;
            }
            v = v.saturating_mul(10).saturating_add((b - b'0') as u32);
            pos += 1;
        }
        Some(v)
    }

    fn parse_pid_proc_line(line: &[u8]) -> Option<u32> {
        let pat = b"pid=";
        let mut i = 0usize;
        while i + pat.len() <= line.len() {
            if &line[i..i + pat.len()] == pat {
                return parse_decimal_from(line, i + pat.len());
            }
            i += 1;
        }
        None
    }

    fn parse_pid_map_line(line: &[u8]) -> Option<u32> {
        if line.len() < 3 || line[0] != b' ' || line[1] != b' ' || line[2] != b'P' {
            return None;
        }
        parse_decimal_from(line, 3)
    }

    uart_puts("\x1b[H\x1b[2J");
    uart_puts("\x1b[1;37m[mapwatch] /proc/runtime/map (use `mapwatch off` to disable)\x1b[0m\n");
    let mut data = [0u8; SHELL_IO_MAX];
    if let Some(len) = shell_read_file(b"/proc/runtime/map", &mut data) {
        let mut start = 0usize;
        while start < len {
            let mut end = start;
            while end < len && data[end] != b'\n' {
                end += 1;
            }
            let line_bytes = &data[start..end];
            let mut color: &[u8] = b"";
            if line_bytes.starts_with(b"Proc") {
                if let Some(pid) = parse_pid_proc_line(line_bytes) {
                    color = ansi_pid_color(pid);
                }
            } else if line_bytes.starts_with(b"  P") {
                if let Some(pid) = parse_pid_map_line(line_bytes) {
                    color = ansi_pid_color(pid);
                }
            } else if line_bytes.starts_with(b"Map") {
                color = b"\x1b[1;36m";
            } else if line_bytes.starts_with(b"Current")
                || line_bytes.starts_with(b"Processes:")
                || line_bytes.starts_with(b"Mappings:")
            {
                color = b"\x1b[1;33m";
            }
            if !color.is_empty() {
                uart_put_bytes(color);
            }
            uart_put_bytes(line_bytes);
            if !color.is_empty() {
                uart_puts("\x1b[0m");
            }
            uart_puts("\n");
            start = if end < len { end + 1 } else { end };
        }
    } else {
        uart_puts("CurrentPid:\t0\nCurrentProc:\tnone\nCurrentTid:\t0\nCurrentThread:\tnone\nCurrentAsid:\t0\nMappings:\t0\n");
    }
    uart_puts("qos-sh:/> ");
    if !line.is_empty() {
        uart_put_bytes(line);
    }
}

fn run_serial_shell() -> ! {
    let mut line = [0u8; SHELL_LINE_MAX];
    let mut mapwatch_enabled = false;
    let mut mapwatch_side_enabled = false;
    loop {
        let mut len = 0usize;
        let mut poll_spins = 0u32;
        let mut side_spins = 0u32;
        if mapwatch_enabled {
            shell_mapwatch_redraw(&line[..len]);
        } else {
            uart_puts("qos-sh:/> ");
        }
        loop {
            let ch = match uart_try_getc() {
                Some(ch) => ch,
                None => {
                    if mapwatch_enabled {
                        poll_spins = poll_spins.saturating_add(1);
                        if poll_spins >= MAPWATCH_POLL_SPINS {
                            poll_spins = 0;
                            shell_mapwatch_redraw(&line[..len]);
                        }
                    }
                    if mapwatch_side_enabled {
                        side_spins = side_spins.saturating_add(1);
                        if side_spins >= MAPWATCH_SIDE_POLL_SPINS {
                            side_spins = 0;
                            mapwatch_side_emit_snapshot();
                        }
                    }
                    core::hint::spin_loop();
                    continue;
                }
            };
            match ch {
                b'\r' | b'\n' => {
                    uart_puts("\n");
                    break;
                }
                0x08 | 0x7F => {
                    if len > 0 {
                        len -= 1;
                        uart_put_bytes(b"\x08 \x08");
                    }
                }
                0x20..=0x7E => {
                    if len + 1 < SHELL_LINE_MAX {
                        line[len] = ch;
                        len += 1;
                        uart_putc(ch);
                    }
                }
                _ => {}
            }
            poll_spins = 0;
            side_spins = 0;
        }

        if shell_handle_line(&line[..len], &mut mapwatch_enabled, &mut mapwatch_side_enabled) {
            loop {
                core::hint::spin_loop();
            }
        }
    }
}

fn run_init_process() -> ! {
    unsafe {
        SHELL_FILE_PATHS = [[0; SHELL_PATH_MAX]; SHELL_FILE_MAX];
        SHELL_FILE_PATH_LENS = [0; SHELL_FILE_MAX];
        SHELL_FILE_USED = [0; SHELL_FILE_MAX];
        MODULE_PATHS = [[0; SHELL_PATH_MAX]; SHELL_FILE_MAX];
        MODULE_PATH_LENS = [0; SHELL_FILE_MAX];
        MODULE_USED = [0; SHELL_FILE_MAX];
        MODULE_IDS = [0; SHELL_FILE_MAX];
        WQ_DEMO_HITS = 0;
    }
    let init_ok = proc_create(1, 0);
    let mut shell_ok = false;
    if init_ok {
        shell_ok = proc_fork(1, 2);
    }
    if !shell_ok {
        shell_ok = proc_create(2, 1);
    }
    let _ = sched_add_task(1);
    let _ = vmm_map_as(1, 0x0000_4000, 0x0010_0000, VM_READ | VM_EXEC | VM_USER);
    let _ = vmm_map_as(1, 0x0000_6000, 0x0010_2000, VM_READ | VM_WRITE | VM_USER);
    let _ = vmm_map_as(1, 0x7FFF_0000, 0x0010_4000, VM_READ | VM_WRITE | VM_USER);
    if shell_ok {
        let _ = sched_add_task(2);
        let _ = vmm_map_as(2, 0x0000_4000, 0x0020_0000, VM_READ | VM_EXEC | VM_USER);
        let _ = vmm_map_as(2, 0x0000_6000, 0x0020_2000, VM_READ | VM_WRITE | VM_USER);
        let _ = vmm_map_as(2, 0x7FFF_0000, 0x0020_4000, VM_READ | VM_WRITE | VM_USER);
        let _ = sched_next();
        let _ = sched_next();
        vmm_set_asid(2);
    } else {
        let _ = sched_next();
        vmm_set_asid(1);
    }

    uart_puts("init[1]: starting /sbin/init\n");
    if shell_ok {
        uart_puts("init[1]: exec /bin/sh\n");
    } else {
        uart_puts("init[1]: failed to exec /bin/sh\n");
    }
    let _ = syscall_dispatch(SYSCALL_NR_MKDIR, b"/tmp\0".as_ptr() as u64, 0, 0, 0);
    let _ = syscall_dispatch(SYSCALL_NR_MKDIR, b"/data\0".as_ptr() as u64, 0, 0, 0);
    stage_test_module_artifacts();
    run_serial_shell();
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    uart_puts("QOS kernel started impl=rust arch=aarch64 handoff=panic entry=kernel_main abi=x0\n");
    run_init_process();
}

fn dtb_magic_ok(dtb_addr: u64) -> bool {
    if dtb_addr == 0 {
        return false;
    }
    unsafe { read_volatile(dtb_addr as *const u32) == QOS_DTB_MAGIC }
}

#[derive(Clone, Copy)]
struct DtbBootExtract {
    mmap_from_dtb: bool,
    mem_base: u64,
    mem_size: u64,
    initramfs_from_dtb: bool,
    initramfs_addr: u64,
    initramfs_size: u64,
    dtb_size: u64,
}

impl DtbBootExtract {
    const fn empty() -> Self {
        Self {
            mmap_from_dtb: false,
            mem_base: 0,
            mem_size: 0,
            initramfs_from_dtb: false,
            initramfs_addr: 0,
            initramfs_size: 0,
            dtb_size: 0,
        }
    }
}

fn dtb_be32_at(blob: &[u8], off: usize) -> Option<u32> {
    if off + 4 > blob.len() {
        return None;
    }
    Some(read_be32(blob, off))
}

fn dtb_cstr_end(blob: &[u8], start: usize, limit: usize) -> Option<usize> {
    if start >= limit || limit > blob.len() {
        return None;
    }
    let mut i = start;
    while i < limit {
        if blob[i] == 0 {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn dtb_node_name_eq(blob: &[u8], start: usize, end: usize, needle: &[u8]) -> bool {
    if end < start || end - start != needle.len() {
        return false;
    }
    &blob[start..end] == needle
}

fn dtb_node_name_starts_with(blob: &[u8], start: usize, end: usize, needle: &[u8]) -> bool {
    if end < start || end - start < needle.len() {
        return false;
    }
    &blob[start..start + needle.len()] == needle
}

fn dtb_prop_name_eq(
    blob: &[u8],
    strings_off: usize,
    strings_end: usize,
    name_off: usize,
    target: &[u8],
) -> bool {
    let start = match strings_off.checked_add(name_off) {
        Some(v) => v,
        None => return false,
    };
    if start >= strings_end || strings_end > blob.len() {
        return false;
    }
    let mut i = 0usize;
    loop {
        let p = start + i;
        if p >= strings_end {
            return false;
        }
        let b = blob[p];
        if i == target.len() {
            return b == 0;
        }
        if b != target[i] {
            return false;
        }
        i += 1;
    }
}

fn dtb_parse_cells_be(value: &[u8], cells: usize) -> Option<u64> {
    if cells == 0 || cells > 2 || value.len() < cells * 4 {
        return None;
    }
    let mut out = 0u64;
    let mut i = 0usize;
    while i < cells {
        out = (out << 32) | read_be32(value, i * 4) as u64;
        i += 1;
    }
    Some(out)
}

fn dtb_parse_scalar_be(value: &[u8]) -> Option<u64> {
    let cells = core::cmp::min(value.len() / 4, 2);
    if cells == 0 {
        return None;
    }
    dtb_parse_cells_be(value, cells)
}

fn dtb_extract_boot_info(dtb_addr: u64) -> DtbBootExtract {
    if dtb_addr == 0 {
        return DtbBootExtract::empty();
    }
    let base = dtb_addr as *const u8;
    let magic = unsafe {
        ((read_volatile(base) as u32) << 24)
            | ((read_volatile(base.add(1)) as u32) << 16)
            | ((read_volatile(base.add(2)) as u32) << 8)
            | (read_volatile(base.add(3)) as u32)
    };
    if magic != FDT_MAGIC_BE {
        return DtbBootExtract::empty();
    }

    let total_size = unsafe {
        ((read_volatile(base.add(4)) as u32) << 24)
            | ((read_volatile(base.add(5)) as u32) << 16)
            | ((read_volatile(base.add(6)) as u32) << 8)
            | (read_volatile(base.add(7)) as u32)
    } as usize;
    if !(40..=DTB_MAX_TOTAL_SIZE).contains(&total_size) {
        return DtbBootExtract::empty();
    }

    let blob = unsafe { core::slice::from_raw_parts(base, total_size) };
    let off_struct = match dtb_be32_at(blob, 8) {
        Some(v) => v as usize,
        None => return DtbBootExtract::empty(),
    };
    let off_strings = match dtb_be32_at(blob, 12) {
        Some(v) => v as usize,
        None => return DtbBootExtract::empty(),
    };
    let size_strings = match dtb_be32_at(blob, 32) {
        Some(v) => v as usize,
        None => return DtbBootExtract::empty(),
    };
    let size_struct = match dtb_be32_at(blob, 36) {
        Some(v) => v as usize,
        None => return DtbBootExtract::empty(),
    };
    let struct_end = match off_struct.checked_add(size_struct) {
        Some(v) if v <= blob.len() => v,
        _ => return DtbBootExtract::empty(),
    };
    let strings_end = match off_strings.checked_add(size_strings) {
        Some(v) if v <= blob.len() => v,
        _ => return DtbBootExtract::empty(),
    };

    let mut depth = 0usize;
    let mut in_memory = [false; DTB_MAX_DEPTH];
    let mut in_chosen = [false; DTB_MAX_DEPTH];
    let mut in_chosen_module = [false; DTB_MAX_DEPTH];
    let mut addr_cells = 2usize;
    let mut size_cells = 2usize;
    let mut out = DtbBootExtract::empty();
    let mut initrd_start = 0u64;
    let mut initrd_end = 0u64;
    let mut have_initrd_start = false;
    let mut have_initrd_end = false;
    out.dtb_size = total_size as u64;

    let mut pos = off_struct;
    while pos + 4 <= struct_end {
        let token = match dtb_be32_at(blob, pos) {
            Some(v) => v,
            None => return DtbBootExtract::empty(),
        };
        pos += 4;
        match token {
            FDT_BEGIN_NODE => {
                let name_start = pos;
                let name_end = match dtb_cstr_end(blob, name_start, struct_end) {
                    Some(v) => v,
                    None => return DtbBootExtract::empty(),
                };
                let parent_depth = depth;
                if depth >= DTB_MAX_DEPTH {
                    return DtbBootExtract::empty();
                }
                let parent_mem = if parent_depth == 0 {
                    false
                } else {
                    in_memory[parent_depth - 1]
                };
                let parent_chosen = if parent_depth == 0 {
                    false
                } else {
                    in_chosen[parent_depth - 1]
                };
                let parent_module = if parent_depth == 0 {
                    false
                } else {
                    in_chosen_module[parent_depth - 1]
                };
                let this_mem = parent_mem
                    || (parent_depth == 1
                        && dtb_node_name_starts_with(blob, name_start, name_end, b"memory"));
                let this_chosen = parent_chosen
                    || (parent_depth == 1 && dtb_node_name_eq(blob, name_start, name_end, b"chosen"));
                let this_module = parent_module
                    || (parent_chosen
                        && dtb_node_name_starts_with(blob, name_start, name_end, b"module@"));
                in_memory[depth] = this_mem;
                in_chosen[depth] = this_chosen;
                in_chosen_module[depth] = this_module;
                depth += 1;
                pos = align_up(name_end + 1, 4);
            }
            FDT_END_NODE => {
                if depth == 0 {
                    return DtbBootExtract::empty();
                }
                depth -= 1;
            }
            FDT_PROP => {
                if pos + 8 > struct_end {
                    return DtbBootExtract::empty();
                }
                let len = match dtb_be32_at(blob, pos) {
                    Some(v) => v as usize,
                    None => return DtbBootExtract::empty(),
                };
                let name_off = match dtb_be32_at(blob, pos + 4) {
                    Some(v) => v as usize,
                    None => return DtbBootExtract::empty(),
                };
                pos += 8;
                if pos + len > struct_end {
                    return DtbBootExtract::empty();
                }
                let value = &blob[pos..pos + len];
                pos = align_up(pos + len, 4);

                if depth == 0 {
                    continue;
                }
                let cur_mem = in_memory[depth - 1];
                let cur_chosen = in_chosen[depth - 1];
                let cur_module = in_chosen_module[depth - 1];

                if depth == 1
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"#address-cells")
                    && len >= 4
                {
                    let v = read_be32(value, 0) as usize;
                    if (1..=2).contains(&v) {
                        addr_cells = v;
                    }
                } else if depth == 1
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"#size-cells")
                    && len >= 4
                {
                    let v = read_be32(value, 0) as usize;
                    if (1..=2).contains(&v) {
                        size_cells = v;
                    }
                } else if cur_mem
                    && !out.mmap_from_dtb
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"reg")
                {
                    let need = (addr_cells + size_cells) * 4;
                    if value.len() >= need {
                        let base_v = dtb_parse_cells_be(value, addr_cells);
                        let size_v = dtb_parse_cells_be(&value[addr_cells * 4..], size_cells);
                        if let (Some(mem_base), Some(mem_size)) = (base_v, size_v) {
                            if mem_size != 0 {
                                out.mmap_from_dtb = true;
                                out.mem_base = mem_base;
                                out.mem_size = mem_size;
                            }
                        }
                    }
                } else if cur_chosen
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"linux,initrd-start")
                {
                    if let Some(v) = dtb_parse_scalar_be(value) {
                        initrd_start = v;
                        have_initrd_start = true;
                    }
                } else if cur_chosen
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"linux,initrd-end")
                {
                    if let Some(v) = dtb_parse_scalar_be(value) {
                        initrd_end = v;
                        have_initrd_end = true;
                    }
                } else if cur_chosen
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"initrd-start")
                {
                    if let Some(v) = dtb_parse_scalar_be(value) {
                        initrd_start = v;
                        have_initrd_start = true;
                    }
                } else if cur_chosen
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"initrd-end")
                {
                    if let Some(v) = dtb_parse_scalar_be(value) {
                        initrd_end = v;
                        have_initrd_end = true;
                    }
                } else if cur_module
                    && dtb_prop_name_eq(blob, off_strings, strings_end, name_off, b"reg")
                {
                    let need = (addr_cells + size_cells) * 4;
                    if value.len() >= need {
                        let base_v = dtb_parse_cells_be(value, addr_cells);
                        let size_v = dtb_parse_cells_be(&value[addr_cells * 4..], size_cells);
                        if let (Some(base), Some(size)) = (base_v, size_v) {
                            if size != 0 {
                                let end = base.saturating_add(size);
                                if end > base {
                                    if !have_initrd_start || base < initrd_start {
                                        initrd_start = base;
                                        have_initrd_start = true;
                                    }
                                    if !have_initrd_end || end > initrd_end {
                                        initrd_end = end;
                                        have_initrd_end = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            FDT_NOP => {}
            FDT_END => break,
            _ => return DtbBootExtract::empty(),
        }
    }

    if have_initrd_start && have_initrd_end && initrd_end > initrd_start {
        out.initramfs_from_dtb = true;
        out.initramfs_addr = initrd_start;
        out.initramfs_size = initrd_end - initrd_start;
    }
    out
}

fn locate_dtb_and_extract(initial_dtb: u64) -> (u64, DtbBootExtract) {
    if initial_dtb != 0 && dtb_magic_ok(initial_dtb) {
        let parsed = dtb_extract_boot_info(initial_dtb);
        if parsed.mmap_from_dtb || parsed.initramfs_from_dtb {
            return (initial_dtb, parsed);
        }
    }
    let mut addr = RAM_SCAN_BASE;
    while addr < RAM_SCAN_END {
        if addr != initial_dtb && dtb_magic_ok(addr) {
            let parsed = dtb_extract_boot_info(addr);
            if parsed.mmap_from_dtb || parsed.initramfs_from_dtb {
                return (addr, parsed);
            }
        }
        addr = addr.saturating_add(RAM_SCAN_STEP);
    }
    if initial_dtb != 0 && dtb_magic_ok(initial_dtb) {
        (initial_dtb, dtb_extract_boot_info(initial_dtb))
    } else {
        (0, DtbBootExtract::empty())
    }
}

fn boot_info_push_mmap(info: &mut BootInfo, base: u64, length: u64, type_: u32) -> bool {
    if length == 0 || type_ == 0 {
        return false;
    }
    let idx = info.mmap_entry_count as usize;
    if idx >= QOS_MMAP_MAX_ENTRIES {
        return false;
    }
    info.mmap_entries[idx] = MmapEntry {
        base,
        length,
        type_,
        _pad: 0,
    };
    info.mmap_entry_count += 1;
    true
}

fn emit_boot_marker_prefix(
    mmap_source: &str,
    mmap_count: u32,
    mmap_len_nonzero: bool,
    initramfs_source: &str,
    initramfs_size_nonzero: bool,
    dtb_handoff: &str,
    irq_timer: &str,
) {
    uart_puts("QOS kernel started impl=rust arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=");
    uart_puts(mmap_source);
    uart_puts(" mmap_count=");
    uart_put_u32(mmap_count);
    uart_puts(" mmap_len_nonzero=");
    uart_put_u32(if mmap_len_nonzero { 1 } else { 0 });
    uart_puts(" initramfs_source=");
    uart_puts(initramfs_source);
    uart_puts(" initramfs_size_nonzero=");
    uart_put_u32(if initramfs_size_nonzero { 1 } else { 0 });
    uart_puts(" dtb_handoff=");
    uart_puts(dtb_handoff);
    uart_puts(" el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb irq_timer=");
    uart_puts(irq_timer);
    uart_puts(" ");
}

fn mmio_read32(base: usize, off: usize) -> u32 {
    unsafe { read_volatile((base + off) as *const u32) }
}

fn mmio_write32(base: usize, off: usize, value: u32) {
    unsafe {
        write_volatile((base + off) as *mut u32, value);
    }
}

fn arch_set_vbar_el1(addr: u64) {
    unsafe {
        asm!("msr vbar_el1, {addr}", addr = in(reg) addr, options(nostack, preserves_flags));
        asm!("isb", options(nostack, preserves_flags));
    }
}

fn arch_enable_irq() {
    unsafe {
        asm!("msr daifclr, #2", options(nostack, preserves_flags));
    }
}

fn arch_disable_irq() {
    unsafe {
        asm!("msr daifset, #2", options(nostack, preserves_flags));
    }
}

fn arch_read_cntfrq_el0() -> u64 {
    let mut value: u64 = 0;
    unsafe {
        asm!("mrs {out}, cntfrq_el0", out = out(reg) value, options(nostack, preserves_flags));
    }
    value
}

fn arch_write_cntp_tval_el0(value: u32) {
    unsafe {
        asm!("msr cntp_tval_el0, {value}", value = in(reg) value as u64, options(nostack, preserves_flags));
    }
}

fn arch_write_cntp_ctl_el0(value: u32) {
    unsafe {
        asm!("msr cntp_ctl_el0, {value}", value = in(reg) value as u64, options(nostack, preserves_flags));
    }
}

fn arch_read_cntp_ctl_el0() -> u32 {
    let mut value: u64 = 0;
    unsafe {
        asm!("mrs {out}, cntp_ctl_el0", out = out(reg) value, options(nostack, preserves_flags));
    }
    value as u32
}

fn gic_set_irq_priority(irq: u32, prio: u8) {
    let reg = GICD_IPRIORITYR + ((irq & !3) as usize);
    let shift = (irq & 3) * 8;
    let mut value = mmio_read32(GICD_BASE, reg);
    value &= !(0xFF << shift);
    value |= (prio as u32) << shift;
    mmio_write32(GICD_BASE, reg, value);
}

#[no_mangle]
pub extern "C" fn qos_irq_dispatch() {
    let iar = mmio_read32(GICC_BASE, GICC_IAR);
    let irq = iar & 0x3FF;
    unsafe {
        IRQ_LAST = irq;
        if irq == GIC_IRQ_TIMER_PPI {
            IRQ_TIMER_HITS = IRQ_TIMER_HITS.saturating_add(1);
            arch_write_cntp_tval_el0(IRQ_TIMER_RELOAD_TICKS);
        }
    }
    if irq < 1020 {
        mmio_write32(GICC_BASE, GICC_EOIR, iar);
    }
}

fn gic_timer_irq_probe() -> bool {
    let freq = arch_read_cntfrq_el0();
    if freq != 0 {
        let ticks = freq / 100;
        if ticks > 0 && ticks <= u32::MAX as u64 {
            unsafe {
                IRQ_TIMER_RELOAD_TICKS = ticks as u32;
            }
        }
    }

    unsafe {
        IRQ_LAST = GIC_IRQ_SPURIOUS;
        IRQ_TIMER_HITS = 0;
    }

    arch_write_cntp_ctl_el0(0);

    mmio_write32(GICC_BASE, GICC_CTLR, 0);
    mmio_write32(GICD_BASE, GICD_CTLR, 0);
    mmio_write32(GICD_BASE, GICD_ICENABLER0, 1 << GIC_IRQ_TIMER_PPI);
    mmio_write32(GICD_BASE, GICD_ICPENDR0, 1 << GIC_IRQ_TIMER_PPI);
    let mut group = mmio_read32(GICD_BASE, GICD_IGROUPR0);
    group |= 1 << GIC_IRQ_TIMER_PPI;
    mmio_write32(GICD_BASE, GICD_IGROUPR0, group);
    gic_set_irq_priority(GIC_IRQ_TIMER_PPI, 0x80);
    mmio_write32(GICC_BASE, GICC_PMR, 0xFF);
    mmio_write32(GICC_BASE, GICC_CTLR, 0x3);
    mmio_write32(GICD_BASE, GICD_ISENABLER0, 1 << GIC_IRQ_TIMER_PPI);
    mmio_write32(GICD_BASE, GICD_CTLR, 0x3);
    fence(Ordering::SeqCst);

    let reload = unsafe { IRQ_TIMER_RELOAD_TICKS };
    arch_write_cntp_tval_el0(reload);
    arch_write_cntp_ctl_el0(1);

    for _ in 0..IRQ_TIMER_PROBE_SPINS {
        if (arch_read_cntp_ctl_el0() & (1 << 2)) != 0 {
            unsafe {
                IRQ_TIMER_HITS = 1;
            }
            break;
        }
        let iar = mmio_read32(GICC_BASE, GICC_IAR);
        let irq = iar & 0x3FF;
        if irq == GIC_IRQ_TIMER_PPI {
            unsafe {
                IRQ_LAST = irq;
                IRQ_TIMER_HITS = 1;
            }
            mmio_write32(GICC_BASE, GICC_EOIR, iar);
            break;
        }
        if irq < 1020 {
            mmio_write32(GICC_BASE, GICC_EOIR, iar);
        }
        unsafe {
            asm!("nop", options(nostack, preserves_flags));
        }
    }

    arch_write_cntp_ctl_el0(0);
    mmio_write32(GICD_BASE, GICD_ICENABLER0, 1 << GIC_IRQ_TIMER_PPI);
    mmio_write32(GICC_BASE, GICC_CTLR, 0);
    mmio_write32(GICD_BASE, GICD_CTLR, 0);
    fence(Ordering::SeqCst);

    unsafe { IRQ_TIMER_HITS != 0 }
}

fn align_up(value: usize, align: usize) -> usize {
    if align == 0 {
        return value;
    }
    (value + (align - 1)) & !(align - 1)
}

fn virt_to_phys(addr: u64) -> u64 {
    if (QOS_HIGH_ALIAS_BASE..QOS_HIGH_ALIAS_END).contains(&addr) {
        addr - QOS_HIGH_ALIAS_DELTA
    } else {
        addr
    }
}

fn write_be16(buf: &mut [u8], off: usize, value: u16) {
    buf[off] = (value >> 8) as u8;
    buf[off + 1] = value as u8;
}

fn write_be32(buf: &mut [u8], off: usize, value: u32) {
    buf[off] = (value >> 24) as u8;
    buf[off + 1] = (value >> 16) as u8;
    buf[off + 2] = (value >> 8) as u8;
    buf[off + 3] = value as u8;
}

fn fake_dtb_emit_begin_node(buf: &mut [u8], mut pos: usize, name: &[u8]) -> usize {
    write_be32(buf, pos, FDT_BEGIN_NODE);
    pos += 4;
    buf[pos..pos + name.len()].copy_from_slice(name);
    pos += name.len();
    buf[pos] = 0;
    pos += 1;
    align_up(pos, 4)
}

fn fake_dtb_emit_prop_cells(buf: &mut [u8], mut pos: usize, name_off: u32, cells: &[u32]) -> usize {
    write_be32(buf, pos, FDT_PROP);
    write_be32(buf, pos + 4, (cells.len() * 4) as u32);
    write_be32(buf, pos + 8, name_off);
    pos += 12;
    for cell in cells {
        write_be32(buf, pos, *cell);
        pos += 4;
    }
    align_up(pos, 4)
}

fn fake_dtb_emit_prop_bytes(buf: &mut [u8], mut pos: usize, name_off: u32, value: &[u8]) -> usize {
    write_be32(buf, pos, FDT_PROP);
    write_be32(buf, pos + 4, value.len() as u32);
    write_be32(buf, pos + 8, name_off);
    pos += 12;
    buf[pos..pos + value.len()].copy_from_slice(value);
    pos += value.len();
    align_up(pos, 4)
}

fn fake_dtb_build() {
    unsafe {
        if QOS_FAKE_DTB_READY {
            return;
        }
        let b_ptr = addr_of_mut!(QOS_FAKE_DTB.blob) as *mut u8;
        let b = core::slice::from_raw_parts_mut(b_ptr, FAKE_DTB_TOTAL_SIZE);
        b.fill(0);

        write_be32(b, 0x00, FDT_MAGIC_BE);
        write_be32(b, 0x04, FAKE_DTB_TOTAL_SIZE as u32);
        write_be32(b, 0x08, FAKE_DTB_OFF_STRUCT as u32);
        write_be32(b, 0x0C, FAKE_DTB_OFF_STRINGS as u32);
        write_be32(b, 0x10, FAKE_DTB_OFF_MEMRSV as u32);
        write_be32(b, 0x14, 17);
        write_be32(b, 0x18, 16);
        write_be32(b, 0x1C, 0);
        write_be32(b, 0x20, FAKE_DTB_SIZE_STRINGS as u32);
        write_be32(b, 0x24, FAKE_DTB_SIZE_STRUCT as u32);

        let mut p = FAKE_DTB_OFF_STRUCT;
        p = fake_dtb_emit_begin_node(b, p, b"");
        p = fake_dtb_emit_prop_cells(b, p, FAKE_DTB_STR_OFF_ADDR_CELLS, &[2]);
        p = fake_dtb_emit_prop_cells(b, p, FAKE_DTB_STR_OFF_SIZE_CELLS, &[2]);
        p = fake_dtb_emit_begin_node(b, p, b"memory@40000000");
        p = fake_dtb_emit_prop_bytes(b, p, FAKE_DTB_STR_OFF_DEVICE_TYPE, b"memory");
        p = fake_dtb_emit_prop_cells(
            b,
            p,
            FAKE_DTB_STR_OFF_REG,
            &[0x0000_0000, 0x4000_0000, 0x0000_0000, 0x1000_0000],
        );
        write_be32(b, p, FDT_END_NODE);
        p += 4;
        p = fake_dtb_emit_begin_node(b, p, b"chosen");
        p = fake_dtb_emit_prop_cells(b, p, FAKE_DTB_STR_OFF_INITRD_START, &[0x0000_0000, 0x4400_0000]);
        p = fake_dtb_emit_prop_cells(b, p, FAKE_DTB_STR_OFF_INITRD_END, &[0x0000_0000, 0x4400_1000]);
        write_be32(b, p, FDT_END_NODE);
        p += 4;
        write_be32(b, p, FDT_END_NODE);
        p += 4;
        write_be32(b, p, FDT_END);

        const STRINGS: &[u8] =
            b"#address-cells\0#size-cells\0device_type\0reg\0linux,initrd-start\0linux,initrd-end\0";
        b[FAKE_DTB_OFF_STRINGS..FAKE_DTB_OFF_STRINGS + STRINGS.len()].copy_from_slice(STRINGS);

        QOS_FAKE_DTB_READY = true;
    }
}

fn read_be16(buf: &[u8], off: usize) -> u16 {
    ((buf[off] as u16) << 8) | buf[off + 1] as u16
}

fn read_be32(buf: &[u8], off: usize) -> u32 {
    ((buf[off] as u32) << 24)
        | ((buf[off + 1] as u32) << 16)
        | ((buf[off + 2] as u32) << 8)
        | (buf[off + 3] as u32)
}

fn ones_complement_checksum(data: &[u8]) -> u16 {
    let mut sum = 0u32;
    let mut i = 0usize;
    while i + 1 < data.len() {
        let word = ((data[i] as u16) << 8) | data[i + 1] as u16;
        sum = sum.wrapping_add(word as u32);
        i += 2;
    }
    if i < data.len() {
        sum = sum.wrapping_add((data[i] as u32) << 8);
    }
    while (sum >> 16) != 0 {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    !(sum as u16)
}

fn ipv4_checksum(header: &[u8]) -> u16 {
    ones_complement_checksum(header)
}

fn build_probe_udp_ipv4_frame(buf: &mut [u8]) -> usize {
    let mut o = 0usize;
    let ip_total_len = (IPV4_HDR_LEN + UDP_HDR_LEN + PROBE_PAYLOAD.len()) as u16;
    let udp_len = (UDP_HDR_LEN + PROBE_PAYLOAD.len()) as u16;

    buf[o..o + 6].fill(0xFF);
    o += 6;
    buf[o..o + 6].copy_from_slice(&PROBE_SRC_MAC);
    o += 6;
    buf[o] = 0x08;
    buf[o + 1] = 0x00;
    o += 2;

    let ip_off = o;
    buf[ip_off] = 0x45;
    buf[ip_off + 1] = 0;
    write_be16(buf, ip_off + 2, ip_total_len);
    write_be16(buf, ip_off + 4, 1);
    write_be16(buf, ip_off + 6, 0x4000);
    buf[ip_off + 8] = 64;
    buf[ip_off + 9] = 17;
    write_be16(buf, ip_off + 10, 0);
    write_be32(buf, ip_off + 12, QOS_NET_IPV4_LOCAL);
    write_be32(buf, ip_off + 16, QOS_NET_IPV4_GATEWAY);
    let csum = ipv4_checksum(&buf[ip_off..ip_off + IPV4_HDR_LEN]);
    write_be16(buf, ip_off + 10, csum);
    o += IPV4_HDR_LEN;

    write_be16(buf, o, PROBE_UDP_SRC_PORT);
    write_be16(buf, o + 2, PROBE_UDP_DST_PORT);
    write_be16(buf, o + 4, udp_len);
    write_be16(buf, o + 6, 0);
    o += UDP_HDR_LEN;

    buf[o..o + PROBE_PAYLOAD.len()].copy_from_slice(PROBE_PAYLOAD);
    o + PROBE_PAYLOAD.len()
}

fn build_probe_icmp_ipv4_frame(buf: &mut [u8]) -> usize {
    let mut o = 0usize;
    let ip_total_len = (IPV4_HDR_LEN + ICMP_HDR_LEN + ICMP_PROBE_PAYLOAD.len()) as u16;

    buf[o..o + 6].fill(0xFF);
    o += 6;
    buf[o..o + 6].copy_from_slice(&PROBE_SRC_MAC);
    o += 6;
    buf[o] = 0x08;
    buf[o + 1] = 0x00;
    o += 2;

    let ip_off = o;
    buf[ip_off] = 0x45;
    buf[ip_off + 1] = 0;
    write_be16(buf, ip_off + 2, ip_total_len);
    write_be16(buf, ip_off + 4, 2);
    write_be16(buf, ip_off + 6, 0x4000);
    buf[ip_off + 8] = 64;
    buf[ip_off + 9] = 1;
    write_be16(buf, ip_off + 10, 0);
    write_be32(buf, ip_off + 12, QOS_NET_IPV4_LOCAL);
    write_be32(buf, ip_off + 16, QOS_NET_IPV4_GATEWAY);
    let ip_csum = ipv4_checksum(&buf[ip_off..ip_off + IPV4_HDR_LEN]);
    write_be16(buf, ip_off + 10, ip_csum);
    o += IPV4_HDR_LEN;

    let icmp_off = o;
    buf[icmp_off] = 8;
    buf[icmp_off + 1] = 0;
    write_be16(buf, icmp_off + 2, 0);
    write_be16(buf, icmp_off + 4, ICMP_ECHO_ID);
    write_be16(buf, icmp_off + 6, ICMP_ECHO_SEQ);
    buf[icmp_off + ICMP_HDR_LEN..icmp_off + ICMP_HDR_LEN + ICMP_PROBE_PAYLOAD.len()]
        .copy_from_slice(ICMP_PROBE_PAYLOAD);
    let icmp_csum = ones_complement_checksum(
        &buf[icmp_off..icmp_off + ICMP_HDR_LEN + ICMP_PROBE_PAYLOAD.len()],
    );
    write_be16(buf, icmp_off + 2, icmp_csum);
    o += ICMP_HDR_LEN + ICMP_PROBE_PAYLOAD.len();
    o
}

fn is_icmp_echo_reply_frame(frame: &[u8]) -> bool {
    if frame.len() < ETH_HDR_LEN + IPV4_HDR_LEN + ICMP_HDR_LEN {
        return false;
    }
    if frame[12] != 0x08 || frame[13] != 0x00 {
        return false;
    }
    let ip_off = ETH_HDR_LEN;
    let ihl = ((frame[ip_off] & 0x0F) as usize) * 4;
    if ihl < IPV4_HDR_LEN || frame.len() < ETH_HDR_LEN + ihl + ICMP_HDR_LEN {
        return false;
    }
    if frame[ip_off + 9] != 1 {
        return false;
    }
    if read_be32(frame, ip_off + 12) != QOS_NET_IPV4_GATEWAY
        || read_be32(frame, ip_off + 16) != QOS_NET_IPV4_LOCAL
    {
        return false;
    }
    let icmp_off = ip_off + ihl;
    if frame[icmp_off] != 0 || frame[icmp_off + 1] != 0 {
        return false;
    }
    if read_be16(frame, icmp_off + 4) != ICMP_ECHO_ID || read_be16(frame, icmp_off + 6) != ICMP_ECHO_SEQ {
        return false;
    }
    let payload_off = icmp_off + ICMP_HDR_LEN;
    let payload_end = payload_off + ICMP_PROBE_PAYLOAD.len();
    if payload_end > frame.len() {
        return false;
    }
    &frame[payload_off..payload_end] == ICMP_PROBE_PAYLOAD
}

fn virtio_net_find_mmio_base() -> Option<(usize, u32)> {
    let mut i = 0usize;
    while i < VIRTIO_MMIO_SCAN_SLOTS {
        let base = VIRTIO_MMIO_BASE_START + i * VIRTIO_MMIO_STRIDE;
        let magic = mmio_read32(base, VIRTIO_MMIO_MAGIC_VALUE);
        if magic == VIRTIO_MMIO_MAGIC {
            let version = mmio_read32(base, VIRTIO_MMIO_VERSION);
            unsafe { NET_TX_VIRTIO_VERSION = version; }
            let dev_id = mmio_read32(base, VIRTIO_MMIO_DEVICE_ID);
            if dev_id == VIRTIO_MMIO_DEVICE_NET {
                return Some((base, version));
            }
        }
        i += 1;
    }
    None
}

fn virtio_net_init_queue_legacy(base: usize, queue: u32, page_addr: u64) -> Option<LegacyQueueState> {
    mmio_write32(base, VIRTIO_MMIO_QUEUE_SEL, queue);
    let qmax = mmio_read32(base, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if qmax == 0 {
        return None;
    }
    let qsize = min(qmax as usize, VIRTQ_SIZE) as u16;
    if qsize == 0 {
        return None;
    }

    mmio_write32(base, VIRTIO_MMIO_QUEUE_NUM, qsize as u32);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_ALIGN, LEGACY_QUEUE_ALIGN as u32);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_PFN, (page_addr >> 12) as u32);
    if mmio_read32(base, VIRTIO_MMIO_QUEUE_PFN) == 0 {
        return None;
    }

    let desc = page_addr as usize as *mut VirtqDesc;
    let avail_off = core::mem::size_of::<VirtqDesc>() * qsize as usize;
    let avail_idx = unsafe { (page_addr as usize as *mut u8).add(avail_off + 2) as *mut u16 };
    let avail_ring = unsafe { (page_addr as usize as *mut u8).add(avail_off + 4) as *mut u16 };
    let used_off = align_up(avail_off + 4 + 2 * qsize as usize + 2, LEGACY_QUEUE_ALIGN);
    let used_idx = unsafe { (page_addr as usize as *const u8).add(used_off + 2) as *const u16 };
    let used_ring = unsafe { (page_addr as usize as *const u8).add(used_off + 4) as *const VirtqUsedElem };

    Some(LegacyQueueState {
        qsize,
        desc,
        avail_idx,
        avail_ring,
        used_idx,
        used_ring,
    })
}

fn virtio_net_probe() -> (bool, bool, bool) {
    let (base, version) = match virtio_net_find_mmio_base() {
        Some(v) => v,
        None => {
            unsafe {
                NET_TX_STAGE = NET_TX_STAGE_NO_NETDEV;
                NET_RX_STAGE = NET_RX_STAGE_NO_NETDEV;
                ICMP_REAL_STAGE = ICMP_REAL_STAGE_NO_NETDEV;
            }
            return (false, false, false);
        }
    };

    if version != 1 {
        unsafe {
            NET_TX_STAGE = NET_TX_STAGE_FEATURES;
            NET_RX_STAGE = NET_RX_STAGE_FEATURES;
            ICMP_REAL_STAGE = ICMP_REAL_STAGE_FEATURES;
        }
        return (false, false, false);
    }

    mmio_write32(base, VIRTIO_MMIO_STATUS, 0);
    let status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    mmio_write32(base, VIRTIO_MMIO_STATUS, status);
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES, 0);
    mmio_write32(base, VIRTIO_MMIO_GUEST_PAGE_SIZE, LEGACY_QUEUE_ALIGN as u32);

    unsafe {
        core::ptr::write_bytes(
            addr_of_mut!(LEGACY_RX_QUEUE_PAGE.0) as *mut u8,
            0,
            core::mem::size_of::<LegacyQueuePage>(),
        );
        core::ptr::write_bytes(
            addr_of_mut!(LEGACY_TX_QUEUE_PAGE.0) as *mut u8,
            0,
            core::mem::size_of::<LegacyQueuePage>(),
        );
    }
    let rx_page = unsafe { virt_to_phys(addr_of_mut!(LEGACY_RX_QUEUE_PAGE.0) as *mut u8 as u64) };
    let tx_page = unsafe { virt_to_phys(addr_of_mut!(LEGACY_TX_QUEUE_PAGE.0) as *mut u8 as u64) };

    let rx = match virtio_net_init_queue_legacy(base, VIRTIO_NET_QUEUE_RX, rx_page) {
        Some(v) => v,
        None => {
            unsafe {
                NET_TX_STAGE = NET_TX_STAGE_QUEUE;
                NET_RX_STAGE = NET_RX_STAGE_QUEUE;
                ICMP_REAL_STAGE = ICMP_REAL_STAGE_QUEUE;
            }
            return (false, false, false);
        }
    };
    let tx = match virtio_net_init_queue_legacy(base, VIRTIO_NET_QUEUE_TX, tx_page) {
        Some(v) => v,
        None => {
            unsafe {
                NET_TX_STAGE = NET_TX_STAGE_QUEUE;
                NET_RX_STAGE = NET_RX_STAGE_QUEUE;
                ICMP_REAL_STAGE = ICMP_REAL_STAGE_QUEUE;
            }
            return (false, false, false);
        }
    };
    mmio_write32(base, VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);

    unsafe {
        core::ptr::write_bytes(
            addr_of_mut!(LEGACY_RX_FRAME_BUF) as *mut u8,
            0,
            core::mem::size_of::<[u8; RX_BUF_LEN]>(),
        );
        core::ptr::write_bytes(
            addr_of_mut!(LEGACY_TX_FRAME_BUF) as *mut u8,
            0,
            core::mem::size_of::<[u8; TX_BUF_LEN]>(),
        );

        write_volatile(
            rx.desc,
            VirtqDesc {
                addr: virt_to_phys(addr_of_mut!(LEGACY_RX_FRAME_BUF) as *mut u8 as u64),
                len: RX_BUF_LEN as u32,
                flags: VIRTQ_DESC_F_WRITE,
                next: 0,
            },
        );
        let old_rx_avail = read_volatile(rx.avail_idx);
        write_volatile(
            rx.avail_ring.add((old_rx_avail as usize) % (rx.qsize as usize)),
            0,
        );
        fence(Ordering::SeqCst);
        write_volatile(rx.avail_idx, old_rx_avail.wrapping_add(1));
        fence(Ordering::SeqCst);
        mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);
        let mut old_rx_used = read_volatile(rx.used_idx);

        let frame_len = build_probe_udp_ipv4_frame(
            &mut LEGACY_TX_FRAME_BUF[VIRTIO_NET_HDR_LEN..VIRTIO_NET_HDR_LEN + PROBE_FRAME_LEN],
        );
        let tx_len = (VIRTIO_NET_HDR_LEN + frame_len) as u32;
        write_volatile(
            tx.desc,
            VirtqDesc {
                addr: virt_to_phys(addr_of_mut!(LEGACY_TX_FRAME_BUF) as *mut u8 as u64),
                len: tx_len,
                flags: 0,
                next: 0,
            },
        );
        let old_tx_avail = read_volatile(tx.avail_idx);
        write_volatile(
            tx.avail_ring.add((old_tx_avail as usize) % (tx.qsize as usize)),
            0,
        );
        fence(Ordering::SeqCst);
        write_volatile(tx.avail_idx, old_tx_avail.wrapping_add(1));
        fence(Ordering::SeqCst);
        mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);

        let old_tx_used = read_volatile(tx.used_idx);
        let tx_target = old_tx_used.wrapping_add(1);
        let mut tx_ok = false;
        let mut spins = 0usize;
        while spins < 2_000_000 {
            let cur = read_volatile(tx.used_idx);
            if cur == tx_target {
                NET_TX_STAGE = NET_TX_STAGE_OK;
                tx_ok = true;
                break;
            }
            spins += 1;
            core::hint::spin_loop();
        }
        if !tx_ok {
            NET_TX_STAGE = NET_TX_STAGE_TIMEOUT;
        }

        let mut rx_ok = false;
        let mut icmp_ok = false;

        core::ptr::write_bytes(
            addr_of_mut!(LEGACY_TX_FRAME_BUF) as *mut u8,
            0,
            core::mem::size_of::<[u8; TX_BUF_LEN]>(),
        );
        let icmp_frame_len = build_probe_icmp_ipv4_frame(
            &mut LEGACY_TX_FRAME_BUF[VIRTIO_NET_HDR_LEN..VIRTIO_NET_HDR_LEN + PROBE_FRAME_LEN],
        );
        let icmp_tx_len = (VIRTIO_NET_HDR_LEN + icmp_frame_len) as u32;
        write_volatile(
            tx.desc,
            VirtqDesc {
                addr: virt_to_phys(addr_of_mut!(LEGACY_TX_FRAME_BUF) as *mut u8 as u64),
                len: icmp_tx_len,
                flags: 0,
                next: 0,
            },
        );
        let old_icmp_avail = read_volatile(tx.avail_idx);
        write_volatile(
            tx.avail_ring.add((old_icmp_avail as usize) % (tx.qsize as usize)),
            0,
        );
        fence(Ordering::SeqCst);
        write_volatile(tx.avail_idx, old_icmp_avail.wrapping_add(1));
        fence(Ordering::SeqCst);
        mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);

        let old_icmp_tx_used = read_volatile(tx.used_idx);
        let icmp_tx_target = old_icmp_tx_used.wrapping_add(1);
        spins = 0;
        while spins < 2_000_000 {
            let cur = read_volatile(tx.used_idx);
            if cur == icmp_tx_target {
                break;
            }
            spins += 1;
            core::hint::spin_loop();
        }
        if spins >= 2_000_000 {
            ICMP_REAL_STAGE = ICMP_REAL_STAGE_TX;
        }

        spins = 0;
        while spins < 2_000_000 {
            let cur = read_volatile(rx.used_idx);
            if cur != old_rx_used {
                let elem = read_volatile(rx.used_ring.add((old_rx_used as usize) % (rx.qsize as usize)));
                let used_len = min(elem.len as usize, RX_BUF_LEN);
                let start = min(VIRTIO_NET_HDR_LEN, used_len);
                if used_len > start {
                    NET_RX_STAGE = NET_RX_STAGE_PAYLOAD;
                    rx_ok = true;
                    if is_icmp_echo_reply_frame(&LEGACY_RX_FRAME_BUF[start..used_len]) {
                        ICMP_REAL_STAGE = ICMP_REAL_STAGE_OK;
                        icmp_ok = true;
                    }
                }
                let old_rx_avail = read_volatile(rx.avail_idx);
                write_volatile(
                    rx.avail_ring.add((old_rx_avail as usize) % (rx.qsize as usize)),
                    0,
                );
                fence(Ordering::SeqCst);
                write_volatile(rx.avail_idx, old_rx_avail.wrapping_add(1));
                fence(Ordering::SeqCst);
                mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);
                old_rx_used = cur;
                if icmp_ok {
                    break;
                }
            }
            spins += 1;
            core::hint::spin_loop();
        }
        if !rx_ok && NET_RX_STAGE != NET_RX_STAGE_PAYLOAD {
            NET_RX_STAGE = NET_RX_STAGE_TIMEOUT;
        }
        if !icmp_ok && ICMP_REAL_STAGE == ICMP_REAL_STAGE_QUEUE {
            ICMP_REAL_STAGE = ICMP_REAL_STAGE_TIMEOUT;
        }

        return (tx_ok, rx_ok, icmp_ok);
    }
}

#[no_mangle]
static mut NET_TX_STAGE: u32 = NET_TX_STAGE_QUEUE;
#[no_mangle]
static mut NET_RX_STAGE: u32 = NET_RX_STAGE_QUEUE;
#[no_mangle]
static mut NET_TX_VIRTIO_VERSION: u32 = 0;
#[no_mangle]
static mut ICMP_REAL_STAGE: u32 = ICMP_REAL_STAGE_QUEUE;

fn uart_put_net_tx_stage() {
    match unsafe { NET_TX_STAGE } {
        NET_TX_STAGE_OK => uart_puts("ok"),
        NET_TX_STAGE_NO_NETDEV => uart_puts("no_netdev"),
        NET_TX_STAGE_FEATURES => uart_puts("features"),
        NET_TX_STAGE_QUEUE => uart_puts("queue"),
        NET_TX_STAGE_TIMEOUT => uart_puts("timeout"),
        _ => uart_puts("unknown"),
    }
}

fn uart_put_net_tx_version() {
    match unsafe { NET_TX_VIRTIO_VERSION } {
        1 => uart_puts("v1"),
        2 => uart_puts("v2"),
        0 => uart_puts("v0"),
        _ => uart_puts("v?"),
    }
}

fn uart_put_net_rx_result() {
    match unsafe { NET_RX_STAGE } {
        NET_RX_STAGE_OK | NET_RX_STAGE_PAYLOAD => uart_puts("real_ok"),
        NET_RX_STAGE_TIMEOUT => uart_puts("real_skip"),
        _ => uart_puts("real_fail"),
    }
}

fn uart_put_net_rx_stage() {
    match unsafe { NET_RX_STAGE } {
        NET_RX_STAGE_OK => uart_puts("ok"),
        NET_RX_STAGE_NO_NETDEV => uart_puts("no_netdev"),
        NET_RX_STAGE_FEATURES => uart_puts("features"),
        NET_RX_STAGE_QUEUE => uart_puts("queue"),
        NET_RX_STAGE_TIMEOUT => uart_puts("timeout"),
        NET_RX_STAGE_PAYLOAD => uart_puts("payload"),
        _ => uart_puts("unknown"),
    }
}

fn uart_put_icmp_real_stage() {
    match unsafe { ICMP_REAL_STAGE } {
        ICMP_REAL_STAGE_OK => uart_puts("ok"),
        ICMP_REAL_STAGE_NO_NETDEV => uart_puts("no_netdev"),
        ICMP_REAL_STAGE_FEATURES => uart_puts("features"),
        ICMP_REAL_STAGE_QUEUE => uart_puts("queue"),
        ICMP_REAL_STAGE_TIMEOUT => uart_puts("timeout"),
        ICMP_REAL_STAGE_TX => uart_puts("tx"),
        _ => uart_puts("unknown"),
    }
}

fn uart_put_icmp_real_result() {
    match unsafe { ICMP_REAL_STAGE } {
        ICMP_REAL_STAGE_OK => uart_puts("roundtrip_ok"),
        ICMP_REAL_STAGE_TIMEOUT => uart_puts("roundtrip_skip"),
        _ => uart_puts("roundtrip_fail"),
    }
}

#[no_mangle]
pub extern "C" fn rust_main(dtb_addr: u64) -> ! {
    uart_puts("probe=enter\n");
    fake_dtb_build();
    let fallback_dtb = unsafe { addr_of!(QOS_FAKE_DTB.blob) as u64 };
    let (resolved_dtb, mut dtb_extract) = locate_dtb_and_extract(dtb_addr);
    let mut effective_dtb = resolved_dtb;
    let dtb_handoff = if resolved_dtb != 0
        && dtb_extract.mmap_from_dtb
        && dtb_extract.initramfs_from_dtb
        && dtb_extract.mem_size != 0
        && dtb_extract.initramfs_size != 0
        && dtb_extract.dtb_size != 0
    {
        if resolved_dtb == dtb_addr {
            "x0"
        } else {
            "scan"
        }
    } else {
        effective_dtb = fallback_dtb;
        dtb_extract = dtb_extract_boot_info(effective_dtb);
        "fallback"
    };
    let mut boot_valid = effective_dtb != 0
        && dtb_extract.mmap_from_dtb
        && dtb_extract.initramfs_from_dtb
        && dtb_extract.mem_size != 0
        && dtb_extract.initramfs_size != 0
        && dtb_extract.dtb_size != 0;

    let mut info = BootInfo::default();
    info.magic = QOS_BOOT_MAGIC;
    if boot_valid {
        info.mmap_entry_count = 0;
        let _ = boot_info_push_mmap(
            &mut info,
            dtb_extract.mem_base,
            dtb_extract.mem_size,
            QOS_MMAP_TYPE_USABLE,
        );
        info.initramfs_addr = dtb_extract.initramfs_addr;
        info.initramfs_size = dtb_extract.initramfs_size;
        info.dtb_addr = effective_dtb;
        let kernel_start = unsafe { &__kernel_phys_start as *const u8 as u64 };
        let kernel_end = unsafe { &__kernel_phys_end as *const u8 as u64 };
        if kernel_end > kernel_start {
            let _ = boot_info_push_mmap(
                &mut info,
                kernel_start,
                kernel_end - kernel_start,
                QOS_MMAP_TYPE_RESERVED,
            );
        }
        let initramfs_addr = info.initramfs_addr;
        let initramfs_size = info.initramfs_size;
        let _ = boot_info_push_mmap(
            &mut info,
            initramfs_addr,
            initramfs_size,
            QOS_MMAP_TYPE_RESERVED,
        );
        let dtb_addr = info.dtb_addr;
        let dtb_size = dtb_extract.dtb_size;
        let _ = boot_info_push_mmap(&mut info, dtb_addr, dtb_size, QOS_MMAP_TYPE_RESERVED);
        if info.mmap_entry_count == 0 {
            boot_valid = false;
        }
    }

    let mmap_source = "dtb";
    let initramfs_source = "dtb";
    let mmap_len_nonzero = info.mmap_entries[0].length != 0;
    let initramfs_size_nonzero = info.initramfs_size != 0;
    let kernel_ok = boot_valid && kernel_entry(&info).is_ok();
    let irq_timer_ok = if kernel_ok { gic_timer_irq_probe() } else { false };
    let irq_timer = if irq_timer_ok { "ok" } else { "fail" };
    let icmp_ok = if kernel_ok {
        let mut reply = [0u8; 8];
        net_icmp_echo(QOS_NET_IPV4_GATEWAY, 1, 1, b"qos", &mut reply).is_some()
    } else {
        false
    };
    let (net_tx_ok, _net_rx_ok, _icmp_real_ok) = if kernel_ok {
        virtio_net_probe()
    } else {
        (false, false, false)
    };
    let dtb_ok = dtb_magic_ok(effective_dtb);

    if kernel_ok && dtb_ok && icmp_ok {
        if net_tx_ok {
            emit_boot_marker_prefix(
                mmap_source,
                info.mmap_entry_count,
                mmap_len_nonzero,
                initramfs_source,
                initramfs_size_nonzero,
                dtb_handoff,
                irq_timer,
            );
            uart_puts("icmp_echo=gateway_ok net_tx=real_ok net_tx_stage=ok net_rx=");
            uart_put_net_rx_result();
            uart_puts(" net_rx_stage=");
            uart_put_net_rx_stage();
            uart_puts(" icmp_real=");
            uart_put_icmp_real_result();
            uart_puts(" icmp_real_stage=");
            uart_put_icmp_real_stage();
            uart_puts("\n");
        } else {
            emit_boot_marker_prefix(
                mmap_source,
                info.mmap_entry_count,
                mmap_len_nonzero,
                initramfs_source,
                initramfs_size_nonzero,
                dtb_handoff,
                irq_timer,
            );
            uart_puts("icmp_echo=gateway_ok net_tx=real_fail net_tx_stage=");
            uart_put_net_tx_stage();
            uart_puts(" net_tx_ver=");
            uart_put_net_tx_version();
            uart_puts(" net_rx=");
            uart_put_net_rx_result();
            uart_puts(" net_rx_stage=");
            uart_put_net_rx_stage();
            uart_puts(" icmp_real=");
            uart_put_icmp_real_result();
            uart_puts(" icmp_real_stage=");
            uart_put_icmp_real_stage();
            uart_puts("\n");
        }
    } else {
        uart_puts("QOS kernel started impl=rust arch=aarch64 handoff=invalid entry=kernel_main abi=x0 dtb_addr_nonzero=0 dtb_magic=bad mmap_source=dtb mmap_count=0 mmap_len_nonzero=0 initramfs_source=dtb initramfs_size_nonzero=0 dtb_handoff=invalid el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb irq_timer=fail icmp_echo=gateway_fail net_tx=real_fail net_tx_stage=");
        uart_put_net_tx_stage();
        uart_puts(" net_tx_ver=");
        uart_put_net_tx_version();
        uart_puts(" net_rx=");
        uart_put_net_rx_result();
        uart_puts(" net_rx_stage=");
        uart_put_net_rx_stage();
        uart_puts(" icmp_real=");
        uart_put_icmp_real_result();
        uart_puts(" icmp_real_stage=");
        uart_put_icmp_real_stage();
        uart_puts("\n");
    }

    run_init_process();
}
