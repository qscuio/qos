use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, sched_add_task, syscall_dispatch, vmm_translate, SYSCALL_NR_ACCEPT, SYSCALL_NR_BIND,
    SYSCALL_NR_CLOSE, SYSCALL_NR_CONNECT, SYSCALL_NR_DUP2, SYSCALL_NR_GETDENTS, SYSCALL_NR_LISTEN,
    SYSCALL_NR_LSEEK, SYSCALL_NR_MKDIR, SYSCALL_NR_MMAP, SYSCALL_NR_MUNMAP, SYSCALL_NR_OPEN,
    SYSCALL_NR_PIPE, SYSCALL_NR_READ, SYSCALL_NR_RECV, SYSCALL_NR_SEND, SYSCALL_NR_SLEEP,
    SYSCALL_NR_SOCKET, SYSCALL_NR_STAT, SYSCALL_NR_WRITE, SYSCALL_NR_YIELD,
};
use std::ffi::CString;
use std::sync::{Mutex, OnceLock};

const QOS_PAGE_SIZE: u64 = 4096;
const SOCK_STREAM: u64 = 1;
const SOCK_DGRAM: u64 = 2;
const SOCK_RAW: u64 = 3;

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

fn valid_boot_info() -> BootInfo {
    let mut info = BootInfo::default();
    info.magic = QOS_BOOT_MAGIC;
    info.mmap_entry_count = 1;
    info.mmap_entries[0] = MmapEntry {
        base: 0x100000,
        length: 0x200000,
        type_: 1,
        _pad: 0,
    };
    info.initramfs_addr = 0x400000;
    info.initramfs_size = 0x1000;
    info
}

#[test]
fn core_remaining_syscall_fd_pipe_io_bridge() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let path = CString::new("/tmp-file").expect("cstring");
    let path_ptr = path.as_ptr() as u64;
    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, path_ptr, 0, 0, 0), 0);
    let mut stat_out = [0xDEADBEEFu64; 2];
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_STAT, path_ptr, stat_out.as_mut_ptr() as u64, 0, 0),
        0
    );
    assert_eq!(stat_out[0], "/tmp-file".len() as u64);

    let fd = syscall_dispatch(SYSCALL_NR_OPEN, path_ptr, 0, 0, 0);
    assert!(fd >= 0);

    let payload = *b"abc";
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_WRITE,
            fd as u64,
            payload.as_ptr() as u64,
            payload.len() as u64,
            0,
        ),
        3
    );

    assert_eq!(syscall_dispatch(SYSCALL_NR_LSEEK, fd as u64, 0, 0, 0), 0);

    let mut out = [0u8; 16];
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_READ,
            fd as u64,
            out.as_mut_ptr() as u64,
            out.len() as u64,
            0,
        ),
        3
    );
    assert_eq!(&out[0..3], b"abc");

    let max_i64 = i64::MAX as u64;
    assert_eq!(syscall_dispatch(SYSCALL_NR_LSEEK, fd as u64, max_i64, 0, 0), i64::MAX);
    assert_eq!(syscall_dispatch(SYSCALL_NR_LSEEK, fd as u64, 1, 1, 0), -1);

    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_GETDENTS,
            fd as u64,
            out.as_mut_ptr() as u64,
            out.len() as u64,
            0,
        ),
        1
    );
    let dents = u32::from_ne_bytes([out[0], out[1], out[2], out[3]]);
    assert_eq!(dents, 1);

    let dupfd = syscall_dispatch(SYSCALL_NR_DUP2, fd as u64, 40, 0, 0);
    assert_eq!(dupfd, 40);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, dupfd as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0), -1);

    let fd2 = syscall_dispatch(SYSCALL_NR_OPEN, path_ptr, 0, 0, 0);
    assert!(fd2 >= 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_LSEEK, fd2 as u64, 0, 0, 0), 0);
    let mut out2 = [0u8; 16];
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_READ,
            fd2 as u64,
            out2.as_mut_ptr() as u64,
            out2.len() as u64,
            0,
        ),
        3
    );
    assert_eq!(&out2[0..3], b"abc");
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd2 as u64, 0, 0, 0), 0);

    let mut fds = [0u32; 2];
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_PIPE, fds.as_mut_ptr() as u64, 0, 0, 0),
        0
    );
    let rfd = fds[0] as u64;
    let wfd = fds[1] as u64;

    let ping = *b"ping";
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_WRITE, wfd, ping.as_ptr() as u64, ping.len() as u64, 0),
        4
    );

    let mut rx = [0u8; 8];
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_READ, rfd, rx.as_mut_ptr() as u64, rx.len() as u64, 0),
        4
    );
    assert_eq!(&rx[0..4], b"ping");

    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, rfd, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, wfd, 0, 0, 0), 0);

    assert_eq!(syscall_dispatch(SYSCALL_NR_OPEN, 0, 0, 0, 0), -1);
}

#[test]
fn core_remaining_syscall_mmap_sched_socket_bridge() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let va = 0x400000u64;
    assert_eq!(syscall_dispatch(SYSCALL_NR_MMAP, va, QOS_PAGE_SIZE, 1, 0), va as i64);
    assert_eq!(vmm_translate(va), Some(va));
    assert_eq!(syscall_dispatch(SYSCALL_NR_MUNMAP, va, QOS_PAGE_SIZE, 0, 0), 0);
    assert_eq!(vmm_translate(va), None);

    assert!(sched_add_task(11));
    assert!(sched_add_task(22));
    let yielded = syscall_dispatch(SYSCALL_NR_YIELD, 0, 0, 0, 0);
    assert!(yielded == 11 || yielded == 22);
    assert_eq!(syscall_dispatch(SYSCALL_NR_SLEEP, 25, 0, 0, 0), 0);

    let mut addr = [0u8; 16];
    addr[2] = 0x1F;
    addr[3] = 0x90; // 8080
    addr[4] = 0x0A;
    addr[5] = 0x00;
    addr[6] = 0x02;
    addr[7] = 0x0F; // 10.0.2.15
    let mut addrlen = 16u32;

    let server = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let client = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let raw = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let raw_icmp = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_RAW, 1, 0);
    let raw_fail = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_RAW, 1, 0);
    let raw_sendto = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_RAW, 1, 0);
    let host_stream = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let no_listener = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    assert!(server >= 0);
    assert!(client >= 0);
    assert!(raw >= 0);
    assert!(raw_icmp >= 0);
    assert!(raw_fail >= 0);
    assert!(raw_sendto >= 0);
    assert!(host_stream >= 0);
    assert!(no_listener >= 0);

    let bad = *b"bad";
    let mut bad_out = [0u8; 8];
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            raw as u64,
            bad.as_ptr() as u64,
            bad.len() as u64,
            0,
        ),
        -1
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            raw as u64,
            bad_out.as_mut_ptr() as u64,
            bad_out.len() as u64,
            0,
        ),
        -1
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_CONNECT,
            no_listener as u64,
            addr.as_mut_ptr() as u64,
            16,
            0,
        ),
        -1
    );

    let mut host_addr = [0u8; 16];
    host_addr[2] = 0x1F;
    host_addr[3] = 0x90; // 8080
    host_addr[4] = 0x0A;
    host_addr[5] = 0x00;
    host_addr[6] = 0x02;
    host_addr[7] = 0x02; // 10.0.2.2
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_CONNECT,
            host_stream as u64,
            host_addr.as_mut_ptr() as u64,
            16,
            0,
        ),
        0
    );
    let http_req = b"GET / HTTP/1.0\r\nHost: 10.0.2.2\r\n\r\n";
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            host_stream as u64,
            http_req.as_ptr() as u64,
            http_req.len() as u64,
            0,
        ),
        http_req.len() as i64
    );
    let mut http_resp = [0u8; 256];
    let got_http = syscall_dispatch(
        SYSCALL_NR_RECV,
        host_stream as u64,
        http_resp.as_mut_ptr() as u64,
        http_resp.len() as u64,
        0,
    );
    assert!(got_http > 0);
    assert!(http_resp[..got_http as usize]
        .windows(b"HTTP/1.0 200 OK".len())
        .any(|w| w == b"HTTP/1.0 200 OK"));

    let mut icmp_addr = [0u8; 16];
    icmp_addr[2] = 0x00;
    icmp_addr[3] = 0x01;
    icmp_addr[4] = 0x0A;
    icmp_addr[5] = 0x00;
    icmp_addr[6] = 0x02;
    icmp_addr[7] = 0x02; // 10.0.2.2 gateway
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_CONNECT,
            raw_icmp as u64,
            icmp_addr.as_mut_ptr() as u64,
            16,
            0,
        ),
        0
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            raw_icmp as u64,
            bad.as_ptr() as u64,
            bad.len() as u64,
            0,
        ),
        3
    );
    let mut src_addr = [0u8; 16];
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            raw_icmp as u64,
            bad_out.as_mut_ptr() as u64,
            bad_out.len() as u64,
            src_addr.as_mut_ptr() as u64,
        ),
        3
    );
    assert_eq!(&bad_out[0..3], b"bad");
    let src_ip =
        ((src_addr[4] as u32) << 24) | ((src_addr[5] as u32) << 16) | ((src_addr[6] as u32) << 8) | src_addr[7] as u32;
    assert_eq!(src_ip, 0x0A000202);

    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            raw_sendto as u64,
            bad.as_ptr() as u64,
            bad.len() as u64,
            icmp_addr.as_mut_ptr() as u64,
        ),
        3
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            raw_sendto as u64,
            bad_out.as_mut_ptr() as u64,
            bad_out.len() as u64,
            src_addr.as_mut_ptr() as u64,
        ),
        3
    );
    assert_eq!(&bad_out[0..3], b"bad");

    let mut icmp_fail_addr = [0u8; 16];
    icmp_fail_addr[2] = 0x00;
    icmp_fail_addr[3] = 0x01;
    icmp_fail_addr[4] = 0x0A;
    icmp_fail_addr[5] = 0x00;
    icmp_fail_addr[6] = 0x02;
    icmp_fail_addr[7] = 0x63; // 10.0.2.99
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_CONNECT,
            raw_fail as u64,
            icmp_fail_addr.as_mut_ptr() as u64,
            16,
            0,
        ),
        0
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            raw_fail as u64,
            bad.as_ptr() as u64,
            bad.len() as u64,
            0,
        ),
        -1
    );

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, server as u64, addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_LISTEN, server as u64, 4, 0, 0), 0);
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_ACCEPT,
            server as u64,
            addr.as_mut_ptr() as u64,
            (&mut addrlen as *mut u32) as u64,
            0,
        ),
        -1
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_CONNECT,
            client as u64,
            addr.as_mut_ptr() as u64,
            16,
            0,
        ),
        0
    );

    let accepted = syscall_dispatch(
        SYSCALL_NR_ACCEPT,
        server as u64,
        addr.as_mut_ptr() as u64,
        (&mut addrlen as *mut u32) as u64,
        0,
    );
    assert!(accepted >= 0);
    assert_eq!(addrlen, 16);
    let peer_port = ((addr[2] as u16) << 8) | addr[3] as u16;
    let peer_ip = ((addr[4] as u32) << 24) | ((addr[5] as u32) << 16) | ((addr[6] as u32) << 8) | addr[7] as u32;
    assert!(peer_port >= 1024);
    assert_eq!(peer_ip, 0x0A00020F);

    let packet = *b"net";
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            client as u64,
            packet.as_ptr() as u64,
            packet.len() as u64,
            0,
        ),
        3
    );

    let mut recv_buf = [0u8; 16];
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            accepted as u64,
            recv_buf.as_mut_ptr() as u64,
            recv_buf.len() as u64,
            0,
        ),
        3
    );
    assert_eq!(&recv_buf[0..3], b"net");

    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, accepted as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, client as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, server as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, raw as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, raw_icmp as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, raw_fail as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, raw_sendto as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, host_stream as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, no_listener as u64, 0, 0, 0), 0);

    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            accepted as u64,
            recv_buf.as_mut_ptr() as u64,
            recv_buf.len() as u64,
            0,
        ),
        -1
    );

    let udp_a = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0);
    let udp_b = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0);
    let udp_c = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0);
    let udp_raw = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0);
    assert!(udp_a >= 0);
    assert!(udp_b >= 0);
    assert!(udp_c >= 0);
    assert!(udp_raw >= 0);

    let mut addr_a = [0u8; 16];
    addr_a[2] = 0x0F;
    addr_a[3] = 0xA1; // 4001
    addr_a[4] = 0x0A;
    addr_a[5] = 0x00;
    addr_a[6] = 0x02;
    addr_a[7] = 0x0B; // 10.0.2.11

    let mut addr_b = [0u8; 16];
    addr_b[2] = 0x0F;
    addr_b[3] = 0xA2; // 4002
    addr_b[4] = 0x0A;
    addr_b[5] = 0x00;
    addr_b[6] = 0x02;
    addr_b[7] = 0x0C; // 10.0.2.12

    let mut addr_c = [0u8; 16];
    addr_c[2] = 0x0F;
    addr_c[3] = 0xA3; // 4003
    addr_c[4] = 0x0A;
    addr_c[5] = 0x00;
    addr_c[6] = 0x02;
    addr_c[7] = 0x0D; // 10.0.2.13

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, udp_a as u64, addr_a.as_mut_ptr() as u64, 4, 0),
        -1
    );
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, udp_a as u64, addr_a.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_LISTEN, udp_a as u64, 1, 0, 0), -1);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, udp_b as u64, addr_b.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, udp_c as u64, addr_c.as_mut_ptr() as u64, 16, 0),
        0
    );
    let mut src_addr_udp = [0u8; 16];
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            udp_a as u64,
            packet.as_ptr() as u64,
            packet.len() as u64,
            addr_b.as_mut_ptr() as u64
        ),
        3
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            udp_b as u64,
            recv_buf.as_mut_ptr() as u64,
            recv_buf.len() as u64,
            src_addr_udp.as_mut_ptr() as u64
        ),
        3
    );
    assert_eq!(&recv_buf[0..3], b"net");
    let udp_src_port = ((src_addr_udp[2] as u16) << 8) | src_addr_udp[3] as u16;
    let udp_src_ip = ((src_addr_udp[4] as u32) << 24)
        | ((src_addr_udp[5] as u32) << 16)
        | ((src_addr_udp[6] as u32) << 8)
        | src_addr_udp[7] as u32;
    assert_eq!(udp_src_port, 4001);
    assert_eq!(udp_src_ip, 0x0A00020F);

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, udp_a as u64, addr_b.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, udp_b as u64, addr_a.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, udp_c as u64, addr_a.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            udp_raw as u64,
            packet.as_ptr() as u64,
            packet.len() as u64,
            0
        ),
        -1
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_SEND,
            udp_a as u64,
            packet.as_ptr() as u64,
            packet.len() as u64,
            0
        ),
        3
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            udp_c as u64,
            recv_buf.as_mut_ptr() as u64,
            recv_buf.len() as u64,
            0
        ),
        -1
    );
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_RECV,
            udp_b as u64,
            recv_buf.as_mut_ptr() as u64,
            recv_buf.len() as u64,
            0
        ),
        3
    );
    assert_eq!(&recv_buf[0..3], b"net");

    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, udp_a as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, udp_b as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, udp_c as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, udp_raw as u64, 0, 0, 0), 0);
}

#[test]
fn core_remaining_socket_ports_reusable_after_close() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let mut udp_addr = [0u8; 16];
    udp_addr[2] = 0x0F;
    udp_addr[3] = 0xA1; // 4001
    udp_addr[4] = 0x0A;
    udp_addr[5] = 0x00;
    udp_addr[6] = 0x02;
    udp_addr[7] = 0x0B; // 10.0.2.11

    let u1 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0);
    assert!(u1 >= 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, u1 as u64, udp_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, u1 as u64, 0, 0, 0), 0);

    let u2 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0);
    assert!(u2 >= 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, u2 as u64, udp_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, u2 as u64, 0, 0, 0), 0);

    let mut tcp_addr = [0u8; 16];
    tcp_addr[2] = 0x1F;
    tcp_addr[3] = 0x90; // 8080
    tcp_addr[4] = 0x0A;
    tcp_addr[5] = 0x00;
    tcp_addr[6] = 0x02;
    tcp_addr[7] = 0x0F; // 10.0.2.15

    let s1 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    assert!(s1 >= 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, s1 as u64, tcp_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_LISTEN, s1 as u64, 1, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, s1 as u64, 0, 0, 0), 0);

    let s2 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    assert!(s2 >= 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, s2 as u64, tcp_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_LISTEN, s2 as u64, 1, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, s2 as u64, 0, 0, 0), 0);
}

#[test]
fn core_remaining_accept_handles_multiple_pending_peers_fifo() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let mut srv_addr = [0u8; 16];
    srv_addr[2] = 0x1F;
    srv_addr[3] = 0x91; // 8081
    srv_addr[4] = 0x0A;
    srv_addr[5] = 0x00;
    srv_addr[6] = 0x02;
    srv_addr[7] = 0x0F;

    let server = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let c1 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let c2 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    assert!(server >= 0 && c1 >= 0 && c2 >= 0);

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, server as u64, srv_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_LISTEN, server as u64, 4, 0, 0), 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, c1 as u64, srv_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, c2 as u64, srv_addr.as_mut_ptr() as u64, 16, 0),
        0
    );

    let mut a1 = [0u8; 16];
    let mut a2 = [0u8; 16];
    let mut a1_len = 16u32;
    let mut a2_len = 16u32;

    let s1 = syscall_dispatch(
        SYSCALL_NR_ACCEPT,
        server as u64,
        a1.as_mut_ptr() as u64,
        (&mut a1_len as *mut u32) as u64,
        0,
    );
    let s2 = syscall_dispatch(
        SYSCALL_NR_ACCEPT,
        server as u64,
        a2.as_mut_ptr() as u64,
        (&mut a2_len as *mut u32) as u64,
        0,
    );
    assert!(s1 >= 0 && s2 >= 0);
    assert_eq!(a1_len, 16);
    assert_eq!(a2_len, 16);
    let p1 = ((a1[2] as u16) << 8) | a1[3] as u16;
    let p2 = ((a2[2] as u16) << 8) | a2[3] as u16;
    assert!(p1 >= 1024);
    assert!(p2 >= 1024);
    assert_ne!(p1, p2);

    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, s1 as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, s2 as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, c1 as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, c2 as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, server as u64, 0, 0, 0), 0);
}

#[test]
fn core_remaining_listen_backlog_limits_pending_connects() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let mut srv_addr = [0u8; 16];
    srv_addr[2] = 0x1F;
    srv_addr[3] = 0x92; // 8082
    srv_addr[4] = 0x0A;
    srv_addr[5] = 0x00;
    srv_addr[6] = 0x02;
    srv_addr[7] = 0x0F;

    let server = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let c1 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    let c2 = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    assert!(server >= 0 && c1 >= 0 && c2 >= 0);

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_BIND, server as u64, srv_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_LISTEN, server as u64, 1, 0, 0), 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, c1 as u64, srv_addr.as_mut_ptr() as u64, 16, 0),
        0
    );
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, c2 as u64, srv_addr.as_mut_ptr() as u64, 16, 0),
        -1
    );

    let mut out = [0u8; 16];
    let mut out_len = 16u32;
    let accepted = syscall_dispatch(
        SYSCALL_NR_ACCEPT,
        server as u64,
        out.as_mut_ptr() as u64,
        (&mut out_len as *mut u32) as u64,
        0,
    );
    assert!(accepted >= 0);

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_CONNECT, c2 as u64, srv_addr.as_mut_ptr() as u64, 16, 0),
        0
    );

    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, accepted as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, c1 as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, c2 as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, server as u64, 0, 0, 0), 0);
}
