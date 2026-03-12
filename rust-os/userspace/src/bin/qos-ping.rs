use qos_kernel::QOS_NET_IPV4_LOCAL;
use qos_userspace::net::{ensure_kernel_booted, fmt_ipv4, parse_ipv4, ping_via_syscall_raw};

fn main() {
    ensure_kernel_booted();
    let host = match std::env::args().nth(1) {
        Some(h) => h,
        None => {
            println!("ping: missing host");
            return;
        }
    };
    let dst_ip = match parse_ipv4(&host) {
        Some(ip) => ip,
        None => {
            println!("ping: invalid host");
            return;
        }
    };
    let payload = [b'q', b'o', b's'];
    if ping_via_syscall_raw(dst_ip, &payload).is_some() {
        println!(
            "PING {host}\n64 bytes from {host}: icmp_seq=1 ttl=64 time=1ms\n1 packets transmitted, 1 received"
        );
    } else {
        println!(
            "PING {host}\nFrom {} icmp_seq=1 Destination Host Unreachable\n1 packets transmitted, 0 received",
            fmt_ipv4(QOS_NET_IPV4_LOCAL)
        );
    }
}
