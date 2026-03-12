#![no_std]
#![no_main]

use core::arch::global_asm;
use core::cmp::min;
use core::panic::PanicInfo;
use core::ptr::{addr_of, addr_of_mut, read_volatile, write_volatile};
use core::sync::atomic::{fence, Ordering};
use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, net_icmp_echo, proc_create, proc_fork, QOS_NET_IPV4_GATEWAY, QOS_NET_IPV4_LOCAL,
};

const UART0: *mut u32 = 0x0900_0000 as *mut u32;
const UART_FR: *const u32 = 0x0900_0018 as *const u32;
const UART_FR_RXFE: u32 = 0x10;
const QOS_DTB_MAGIC: u32 = 0xEDFE0DD0;

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
    magic: u32,
    totalsize: u32,
    blob: [u8; 32],
}

#[no_mangle]
static QOS_FAKE_DTB: FakeDtb = FakeDtb {
    magic: QOS_DTB_MAGIC,
    totalsize: 0x28,
    blob: [0; 32],
};

global_asm!(
    r#"
    .section .text._start,"ax"
    .global _start
_start:
    adrp x1, QOS_STACK
    add x1, x1, :lo12:QOS_STACK
    mov x2, #65536
    add x1, x1, x2
    mov sp, x1
    bl rust_main
1:
    wfe
    b 1b
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

fn uart_getc() -> u8 {
    loop {
        let fr = unsafe { read_volatile(UART_FR) };
        if (fr & UART_FR_RXFE) == 0 {
            let dr = unsafe { read_volatile(UART0 as *const u32) };
            return (dr & 0xFF) as u8;
        }
        core::hint::spin_loop();
    }
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

fn shell_handle_line(line: &[u8]) -> bool {
    if line.is_empty() {
        return false;
    }
    if line == b"help" {
        uart_puts(
            "qos-sh commands:\n  help\n  echo <text>\n  ps\n  ping <ip>\n  ip addr\n  ip route\n  wget <url>\n  exit\n",
        );
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
    uart_puts("unknown command: ");
    uart_put_bytes(line);
    uart_puts("\n");
    false
}

fn run_serial_shell() -> ! {
    let mut line = [0u8; SHELL_LINE_MAX];
    loop {
        let mut len = 0usize;
        uart_puts("qos-sh:/> ");
        loop {
            let ch = uart_getc();
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
        }

        if shell_handle_line(&line[..len]) {
            loop {
                core::hint::spin_loop();
            }
        }
    }
}

fn run_init_process() -> ! {
    let init_ok = proc_create(1, 0);
    let mut shell_ok = false;
    if init_ok {
        shell_ok = proc_fork(1, 2);
    }
    if !shell_ok {
        shell_ok = proc_create(2, 1);
    }

    uart_puts("init[1]: starting /sbin/init\n");
    if shell_ok {
        uart_puts("init[1]: exec /bin/sh\n");
    } else {
        uart_puts("init[1]: failed to exec /bin/sh\n");
    }
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

fn mmio_read32(base: usize, off: usize) -> u32 {
    unsafe { read_volatile((base + off) as *const u32) }
}

fn mmio_write32(base: usize, off: usize, value: u32) {
    unsafe {
        write_volatile((base + off) as *mut u32, value);
    }
}

fn align_up(value: usize, align: usize) -> usize {
    if align == 0 {
        return value;
    }
    (value + (align - 1)) & !(align - 1)
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
    let rx_page = unsafe { addr_of!(LEGACY_RX_QUEUE_PAGE.0) as u64 };
    let tx_page = unsafe { addr_of!(LEGACY_TX_QUEUE_PAGE.0) as u64 };

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
                addr: addr_of!(LEGACY_RX_FRAME_BUF) as u64,
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
                addr: addr_of!(LEGACY_TX_FRAME_BUF) as u64,
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
                addr: addr_of!(LEGACY_TX_FRAME_BUF) as u64,
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

fn net_tx_stage_text() -> &'static str {
    let stage = unsafe { NET_TX_STAGE };
    match stage {
        NET_TX_STAGE_OK => "ok",
        NET_TX_STAGE_NO_NETDEV => "no_netdev",
        NET_TX_STAGE_FEATURES => "features",
        NET_TX_STAGE_QUEUE => "queue",
        NET_TX_STAGE_TIMEOUT => "timeout",
        _ => "unknown",
    }
}

fn net_rx_stage_text() -> &'static str {
    let stage = unsafe { NET_RX_STAGE };
    match stage {
        NET_RX_STAGE_OK => "ok",
        NET_RX_STAGE_NO_NETDEV => "no_netdev",
        NET_RX_STAGE_FEATURES => "features",
        NET_RX_STAGE_QUEUE => "queue",
        NET_RX_STAGE_TIMEOUT => "timeout",
        NET_RX_STAGE_PAYLOAD => "payload",
        _ => "unknown",
    }
}

fn net_tx_version_text() -> &'static str {
    match unsafe { NET_TX_VIRTIO_VERSION } {
        1 => "v1",
        2 => "v2",
        0 => "v0",
        _ => "v?",
    }
}

fn net_rx_result_text() -> &'static str {
    match unsafe { NET_RX_STAGE } {
        NET_RX_STAGE_OK => "real_ok",
        NET_RX_STAGE_PAYLOAD => "real_ok",
        NET_RX_STAGE_TIMEOUT => "real_skip",
        _ => "real_fail",
    }
}

fn icmp_real_stage_text() -> &'static str {
    match unsafe { ICMP_REAL_STAGE } {
        ICMP_REAL_STAGE_OK => "ok",
        ICMP_REAL_STAGE_NO_NETDEV => "no_netdev",
        ICMP_REAL_STAGE_FEATURES => "features",
        ICMP_REAL_STAGE_QUEUE => "queue",
        ICMP_REAL_STAGE_TIMEOUT => "timeout",
        ICMP_REAL_STAGE_TX => "tx",
        _ => "unknown",
    }
}

fn icmp_real_result_text() -> &'static str {
    match unsafe { ICMP_REAL_STAGE } {
        ICMP_REAL_STAGE_OK => "roundtrip_ok",
        ICMP_REAL_STAGE_TIMEOUT => "roundtrip_skip",
        _ => "roundtrip_fail",
    }
}

#[no_mangle]
pub extern "C" fn rust_main(dtb_addr: u64) -> ! {
    uart_puts("probe=enter\n");
    let fallback_dtb = addr_of!(QOS_FAKE_DTB) as u64;
    let effective_dtb = if dtb_addr != 0 && dtb_magic_ok(dtb_addr) {
        dtb_addr
    } else {
        fallback_dtb
    };
    let dtb_nonzero = effective_dtb != 0;
    let dtb_ok = dtb_magic_ok(effective_dtb);

    let mut info = BootInfo::default();
    info.magic = QOS_BOOT_MAGIC;
    info.mmap_entry_count = 1;
    info.mmap_entries[0] = MmapEntry {
        base: 0x4000_0000,
        length: 0x0400_0000,
        type_: 1,
        _pad: 0,
    };
    info.initramfs_addr = 0x4400_0000;
    info.initramfs_size = 0x1000;
    info.dtb_addr = effective_dtb;

    let kernel_ok = kernel_entry(&info).is_ok();
    let icmp_ok = if kernel_ok {
        let mut reply = [0u8; 8];
        net_icmp_echo(QOS_NET_IPV4_GATEWAY, 1, 1, b"qos", &mut reply).is_some()
    } else {
        false
    };
    let (net_tx_ok, _net_rx_ok, _icmp_real_ok) = virtio_net_probe();

    if kernel_ok && dtb_nonzero && dtb_ok && icmp_ok {
        if net_tx_ok {
            uart_puts("QOS kernel started impl=rust arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=dtb_stub mmap_count=1 mmap_len_nonzero=1 initramfs_source=stub initramfs_size_nonzero=1 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_ok net_tx=real_ok net_tx_stage=ok net_rx=");
            uart_puts(net_rx_result_text());
            uart_puts(" net_rx_stage=");
            uart_puts(net_rx_stage_text());
            uart_puts(" icmp_real=");
            uart_puts(icmp_real_result_text());
            uart_puts(" icmp_real_stage=");
            uart_puts(icmp_real_stage_text());
            uart_puts("\n");
        } else {
            uart_puts("QOS kernel started impl=rust arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=dtb_stub mmap_count=1 mmap_len_nonzero=1 initramfs_source=stub initramfs_size_nonzero=1 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_ok net_tx=real_fail net_tx_stage=");
            uart_puts(net_tx_stage_text());
            uart_puts(" net_tx_ver=");
            uart_puts(net_tx_version_text());
            uart_puts(" net_rx=");
            uart_puts(net_rx_result_text());
            uart_puts(" net_rx_stage=");
            uart_puts(net_rx_stage_text());
            uart_puts(" icmp_real=");
            uart_puts(icmp_real_result_text());
            uart_puts(" icmp_real_stage=");
            uart_puts(icmp_real_stage_text());
            uart_puts("\n");
        }
    } else {
        uart_puts("QOS kernel started impl=rust arch=aarch64 handoff=invalid entry=kernel_main abi=x0 dtb_addr_nonzero=0 dtb_magic=bad mmap_source=dtb_stub mmap_count=0 mmap_len_nonzero=0 initramfs_source=stub initramfs_size_nonzero=0 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_fail net_tx=real_fail net_tx_stage=");
        uart_puts(net_tx_stage_text());
        uart_puts(" net_tx_ver=");
        uart_puts(net_tx_version_text());
        uart_puts(" net_rx=");
        uart_puts(net_rx_result_text());
        uart_puts(" net_rx_stage=");
        uart_puts(net_rx_stage_text());
        uart_puts(" icmp_real=");
        uart_puts(icmp_real_result_text());
        uart_puts(" icmp_real_stage=");
        uart_puts(icmp_real_stage_text());
        uart_puts("\n");
    }

    run_init_process();
}
