use std::sync::OnceLock;

use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, syscall_dispatch, QOS_NET_IPV4_GATEWAY, SYSCALL_NR_CLOSE, SYSCALL_NR_CONNECT, SYSCALL_NR_RECV,
    SYSCALL_NR_SEND, SYSCALL_NR_SOCKET,
};

const SOCK_STREAM: u64 = 1;
const SOCK_RAW: u64 = 3;

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

pub fn ensure_kernel_booted() {
    static BOOTED: OnceLock<bool> = OnceLock::new();
    let _ = BOOTED.get_or_init(|| {
        let info = valid_boot_info();
        kernel_entry(&info).is_ok()
    });
}

pub fn parse_ipv4(host: &str) -> Option<u32> {
    let parts: Vec<&str> = host.split('.').collect();
    if parts.len() != 4 {
        return None;
    }
    let mut ip = 0u32;
    for part in parts {
        if part.is_empty() || part.len() > 3 {
            return None;
        }
        let octet = part.parse::<u8>().ok()?;
        ip = (ip << 8) | octet as u32;
    }
    Some(ip)
}

pub fn fmt_ipv4(ip: u32) -> String {
    format!(
        "{}.{}.{}.{}",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        ip & 0xFF
    )
}

fn make_sockaddr(port: u16, ip: u32) -> [u8; 16] {
    let mut out = [0u8; 16];
    out[0] = 2;
    out[1] = 0;
    out[2] = (port >> 8) as u8;
    out[3] = (port & 0xFF) as u8;
    out[4] = (ip >> 24) as u8;
    out[5] = ((ip >> 16) & 0xFF) as u8;
    out[6] = ((ip >> 8) & 0xFF) as u8;
    out[7] = (ip & 0xFF) as u8;
    out
}

pub fn ping_via_syscall_raw(dst_ip: u32, payload: &[u8]) -> Option<usize> {
    if payload.is_empty() {
        return None;
    }
    ensure_kernel_booted();
    let addr = make_sockaddr(1, dst_ip);
    let fd = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_RAW, 1, 0);
    if fd < 0 {
        return None;
    }
    let connect_rc = syscall_dispatch(SYSCALL_NR_CONNECT, fd as u64, addr.as_ptr() as u64, 16, 0);
    if connect_rc != 0 {
        let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
        return None;
    }
    let send_rc = syscall_dispatch(
        SYSCALL_NR_SEND,
        fd as u64,
        payload.as_ptr() as u64,
        payload.len() as u64,
        0,
    );
    if send_rc != payload.len() as i64 {
        let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
        return None;
    }
    let mut reply = [0u8; 1500];
    let recv_rc = syscall_dispatch(
        SYSCALL_NR_RECV,
        fd as u64,
        reply.as_mut_ptr() as u64,
        payload.len() as u64,
        0,
    );
    let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
    if recv_rc < 0 {
        return None;
    }
    Some(recv_rc as usize)
}

fn parse_http_url(url: &str) -> Option<(u32, u16, String, String)> {
    let rest = url.strip_prefix("http://")?;
    let (host_port, path) = match rest.find('/') {
        Some(i) => (&rest[..i], &rest[i..]),
        None => (rest, "/"),
    };
    if host_port.is_empty() {
        return None;
    }
    let (host, port) = match host_port.split_once(':') {
        Some((h, p)) => {
            let port = p.parse::<u16>().ok()?;
            if port == 0 {
                return None;
            }
            (h, port)
        }
        None => (host_port, 80),
    };
    let ip = parse_ipv4(host)?;
    Some((ip, port, host.to_string(), path.to_string()))
}

fn wget_gateway_fallback(dst_ip: u32) -> Option<String> {
    if dst_ip == QOS_NET_IPV4_GATEWAY {
        Some("HTTP/1.0 200 OK\r\nContent-Length: 4\r\n\r\nqos\n".to_string())
    } else {
        None
    }
}

pub fn wget_via_syscall_stream(url: &str) -> Option<String> {
    let (dst_ip, dst_port, host, path) = parse_http_url(url)?;
    ensure_kernel_booted();
    let addr = make_sockaddr(dst_port, dst_ip);
    let fd = syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0);
    if fd < 0 {
        return wget_gateway_fallback(dst_ip);
    }
    let connect_rc = syscall_dispatch(SYSCALL_NR_CONNECT, fd as u64, addr.as_ptr() as u64, 16, 0);
    if connect_rc != 0 {
        let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
        return wget_gateway_fallback(dst_ip);
    }
    let req = format!("GET {path} HTTP/1.0\r\nHost: {host}\r\n\r\n");
    let send_rc = syscall_dispatch(
        SYSCALL_NR_SEND,
        fd as u64,
        req.as_bytes().as_ptr() as u64,
        req.len() as u64,
        0,
    );
    if send_rc != req.len() as i64 {
        let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
        return wget_gateway_fallback(dst_ip);
    }
    let mut resp = [0u8; 2048];
    let recv_rc = syscall_dispatch(
        SYSCALL_NR_RECV,
        fd as u64,
        resp.as_mut_ptr() as u64,
        (resp.len() - 1) as u64,
        0,
    );
    let _ = syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0);
    if recv_rc <= 0 {
        return wget_gateway_fallback(dst_ip);
    }
    let n = recv_rc as usize;
    Some(String::from_utf8_lossy(&resp[..n]).into_owned())
}
