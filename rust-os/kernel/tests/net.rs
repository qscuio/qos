use qos_kernel::{
    net_arp_count, net_arp_lookup, net_arp_put, net_arp_tick, net_dequeue_packet, net_enqueue_packet,
    net_icmp_echo, net_ipv4_route, net_queue_len, net_reset, net_tcp_connect, net_tcp_listen, net_tcp_retries,
    net_tcp_rto, net_tcp_state, net_tcp_step, net_tcp_next_rto_ms, net_udp_bind, net_udp_recv,
    net_udp_send,
};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

#[test]
fn net_enqueue_dequeue_fifo() {
    let _guard = test_guard();
    net_reset();
    assert!(net_enqueue_packet(&[1, 2, 3, 4]));
    assert!(net_enqueue_packet(&[9, 8, 7]));
    assert_eq!(net_queue_len(), 2);
    let mut out = [0u8; 16];
    assert_eq!(net_dequeue_packet(&mut out), Some(4));
    assert_eq!(&out[..4], [1, 2, 3, 4]);
    assert_eq!(net_dequeue_packet(&mut out), Some(3));
    assert_eq!(&out[..3], [9, 8, 7]);
    assert_eq!(net_queue_len(), 0);
}

#[test]
fn net_rejects_invalid_sizes() {
    let _guard = test_guard();
    net_reset();
    assert!(!net_enqueue_packet(&[]));
    assert!(!net_enqueue_packet(&[0u8; 1501]));
    assert!(net_enqueue_packet(&[1, 2]));
    let mut out = [0u8; 1];
    assert_eq!(net_dequeue_packet(&mut out), None);
}

#[test]
fn net_arp_cache_ttl_and_lookup() {
    let _guard = test_guard();
    net_reset();

    let ip = 0x0A000202u32; // 10.0.2.2
    let mac = [0x52, 0x54, 0x00, 0x12, 0x34, 0x56];
    assert!(net_arp_put(ip, &mac, 30));
    assert_eq!(net_arp_count(), 1);
    assert_eq!(net_arp_lookup(ip), Some(mac));
    net_arp_tick(29);
    assert_eq!(net_arp_lookup(ip), Some(mac));
    net_arp_tick(1);
    assert_eq!(net_arp_lookup(ip), None);
    assert_eq!(net_arp_count(), 0);
}

#[test]
fn net_ipv4_route_selection() {
    let _guard = test_guard();
    net_reset();

    assert_eq!(net_ipv4_route(0x0A000263), Some(0x0A000263)); // 10.0.2.99
    assert_eq!(net_ipv4_route(0x01010101), Some(0x0A000202)); // via gateway
    assert_eq!(net_ipv4_route(0), None);
}

#[test]
fn net_icmp_echo_gateway_and_unreachable() {
    let _guard = test_guard();
    net_reset();

    let payload = [b'q', b'o', b's'];
    let mut reply = [0u8; 8];

    assert_eq!(
        net_icmp_echo(0x0A000202, 1, 1, &payload, &mut reply),
        Some((3, 0x0A000202))
    );
    assert_eq!(&reply[..3], [b'q', b'o', b's']);
    assert_eq!(net_icmp_echo(0x0A000263, 1, 1, &payload, &mut reply), None);
}

#[test]
fn net_udp_demux_by_port() {
    let _guard = test_guard();
    net_reset();

    assert!(net_udp_bind(8080));
    assert!(!net_udp_bind(8080));
    assert_eq!(net_udp_send(5353, 0x0A00020F, 8080, &[1, 2, 3]), Some(3));

    let mut out = [0u8; 8];
    assert_eq!(net_udp_recv(8080, &mut out), Some((3, 0x0A00020F, 5353)));
    assert_eq!(&out[..3], [1, 2, 3]);
    assert_eq!(net_udp_recv(8080, &mut out), None);
}

#[test]
fn net_rejects_low_ports_for_dynamic_use() {
    let _guard = test_guard();
    net_reset();

    assert!(!net_udp_bind(53));
    assert!(net_udp_bind(8081));
    assert_eq!(net_udp_send(53, 0x0A00020F, 8081, &[7]), None);

    assert!(net_tcp_listen(80).is_none());
    assert!(net_tcp_listen(8080).is_some());
    assert!(net_tcp_connect(80, 0x0A000202, 8080).is_none());
}

#[test]
fn net_tcp_state_machine() {
    let _guard = test_guard();
    net_reset();

    const TCP_SYN_SENT: u32 = 2;
    const TCP_ESTABLISHED: u32 = 4;
    const TCP_FIN_WAIT_1: u32 = 5;
    const TCP_FIN_WAIT_2: u32 = 6;
    const TCP_TIME_WAIT: u32 = 10;
    const TCP_CLOSED: u32 = 0;
    const EVT_RX_SYN_ACK: u32 = 2;
    const EVT_APP_CLOSE: u32 = 4;
    const EVT_RX_ACK: u32 = 3;
    const EVT_RX_FIN: u32 = 5;
    const EVT_TIMEOUT: u32 = 6;

    assert!(net_tcp_listen(8080).is_some());
    let conn = net_tcp_connect(50000, 0x0A000202, 8080).expect("connect id");
    assert_eq!(net_tcp_state(conn), Some(TCP_SYN_SENT));
    assert!(net_tcp_step(conn, EVT_RX_SYN_ACK));
    assert_eq!(net_tcp_state(conn), Some(TCP_ESTABLISHED));
    assert!(net_tcp_step(conn, EVT_APP_CLOSE));
    assert_eq!(net_tcp_state(conn), Some(TCP_FIN_WAIT_1));
    assert!(net_tcp_step(conn, EVT_RX_ACK));
    assert_eq!(net_tcp_state(conn), Some(TCP_FIN_WAIT_2));
    assert!(net_tcp_step(conn, EVT_RX_FIN));
    assert_eq!(net_tcp_state(conn), Some(TCP_TIME_WAIT));
    assert!(net_tcp_step(conn, EVT_TIMEOUT));
    assert_eq!(net_tcp_state(conn), Some(TCP_CLOSED));
}

#[test]
fn net_tcp_timeout_backoff_and_retry_limit() {
    let _guard = test_guard();
    net_reset();

    const TCP_SYN_SENT: u32 = 2;
    const TCP_CLOSED: u32 = 0;
    const EVT_TIMEOUT: u32 = 6;

    assert!(net_tcp_listen(8080).is_some());
    let conn = net_tcp_connect(50000, 0x0A000202, 8080).expect("connect id");
    assert_eq!(net_tcp_state(conn), Some(TCP_SYN_SENT));
    assert_eq!(net_tcp_rto(conn), Some(500));
    assert_eq!(net_tcp_retries(conn), Some(0));

    assert!(net_tcp_step(conn, EVT_TIMEOUT));
    assert_eq!(net_tcp_state(conn), Some(TCP_SYN_SENT));
    assert_eq!(net_tcp_rto(conn), Some(1000));
    assert_eq!(net_tcp_retries(conn), Some(1));

    assert!(net_tcp_step(conn, EVT_TIMEOUT));
    assert_eq!(net_tcp_rto(conn), Some(2000));
    assert_eq!(net_tcp_retries(conn), Some(2));

    assert!(net_tcp_step(conn, EVT_TIMEOUT));
    assert_eq!(net_tcp_rto(conn), Some(4000));
    assert_eq!(net_tcp_retries(conn), Some(3));

    assert!(net_tcp_step(conn, EVT_TIMEOUT));
    assert_eq!(net_tcp_rto(conn), Some(8000));
    assert_eq!(net_tcp_retries(conn), Some(4));

    assert!(net_tcp_step(conn, EVT_TIMEOUT));
    assert_eq!(net_tcp_rto(conn), Some(16000));
    assert_eq!(net_tcp_retries(conn), Some(5));

    // After 5 retries, the next timeout closes the connection.
    assert!(net_tcp_step(conn, EVT_TIMEOUT));
    assert_eq!(net_tcp_state(conn), Some(TCP_CLOSED));
}

#[test]
fn net_tcp_rto_helper_caps_at_60s() {
    assert_eq!(net_tcp_next_rto_ms(500), 1000);
    assert_eq!(net_tcp_next_rto_ms(1000), 2000);
    assert_eq!(net_tcp_next_rto_ms(30001), 60000);
    assert_eq!(net_tcp_next_rto_ms(60000), 60000);
    assert_eq!(net_tcp_next_rto_ms(70000), 60000);
}
