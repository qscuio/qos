from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_net_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_net.so"
    out.parent.mkdir(parents=True, exist_ok=True)
    build = _run(
        [
            "gcc",
            "-shared",
            "-fPIC",
            "-std=c11",
            "-O2",
            "-I",
            str(ROOT / "c-os"),
            "-o",
            str(out),
            str(ROOT / "c-os/kernel/init_state.c"),
            str(ROOT / "c-os/kernel/net/net.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def test_c_net_enqueue_dequeue_fifo() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_enqueue_packet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_net_enqueue_packet.restype = ctypes.c_int
    lib.qos_net_dequeue_packet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_net_dequeue_packet.restype = ctypes.c_int
    lib.qos_net_queue_len.argtypes = []
    lib.qos_net_queue_len.restype = ctypes.c_uint32

    p1 = (ctypes.c_ubyte * 4)(1, 2, 3, 4)
    p2 = (ctypes.c_ubyte * 3)(9, 8, 7)
    out = (ctypes.c_ubyte * 16)()

    lib.qos_net_reset()
    assert lib.qos_net_enqueue_packet(p1, 4) == 0
    assert lib.qos_net_enqueue_packet(p2, 3) == 0
    assert lib.qos_net_queue_len() == 2

    n1 = lib.qos_net_dequeue_packet(out, 16)
    assert n1 == 4
    assert list(out)[:4] == [1, 2, 3, 4]
    n2 = lib.qos_net_dequeue_packet(out, 16)
    assert n2 == 3
    assert list(out)[:3] == [9, 8, 7]
    assert lib.qos_net_queue_len() == 0


def test_c_net_rejects_invalid_sizes() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_enqueue_packet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_net_enqueue_packet.restype = ctypes.c_int
    lib.qos_net_dequeue_packet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_net_dequeue_packet.restype = ctypes.c_int

    tiny = (ctypes.c_ubyte * 2)(1, 2)
    out = (ctypes.c_ubyte * 2)()

    lib.qos_net_reset()
    assert lib.qos_net_enqueue_packet(tiny, 0) != 0
    assert lib.qos_net_enqueue_packet(tiny, 1501) != 0
    assert lib.qos_net_enqueue_packet(tiny, 2) == 0
    assert lib.qos_net_dequeue_packet(out, 1) != 0


def test_c_net_arp_cache_ttl_and_lookup() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_arp_put.argtypes = [ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32]
    lib.qos_net_arp_put.restype = ctypes.c_int
    lib.qos_net_arp_lookup.argtypes = [ctypes.c_uint32, ctypes.c_void_p]
    lib.qos_net_arp_lookup.restype = ctypes.c_int
    lib.qos_net_arp_tick.argtypes = [ctypes.c_uint32]
    lib.qos_net_arp_tick.restype = None
    lib.qos_net_arp_count.argtypes = []
    lib.qos_net_arp_count.restype = ctypes.c_uint32

    ip = 0x0A000202  # 10.0.2.2
    mac = (ctypes.c_ubyte * 6)(0x52, 0x54, 0x00, 0x12, 0x34, 0x56)
    out = (ctypes.c_ubyte * 6)()

    lib.qos_net_reset()
    assert lib.qos_net_arp_put(ip, mac, 30) == 0
    assert lib.qos_net_arp_count() == 1
    assert lib.qos_net_arp_lookup(ip, out) == 0
    assert list(out) == [0x52, 0x54, 0x00, 0x12, 0x34, 0x56]
    lib.qos_net_arp_tick(29)
    assert lib.qos_net_arp_lookup(ip, out) == 0
    lib.qos_net_arp_tick(1)
    assert lib.qos_net_arp_lookup(ip, out) == -1
    assert lib.qos_net_arp_count() == 0


def test_c_net_ipv4_route_selection() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_ipv4_route.argtypes = [ctypes.c_uint32, ctypes.c_void_p]
    lib.qos_net_ipv4_route.restype = ctypes.c_int

    next_hop = ctypes.c_uint32(0)
    lib.qos_net_reset()

    assert lib.qos_net_ipv4_route(0x0A000263, ctypes.byref(next_hop)) == 0  # 10.0.2.99
    assert next_hop.value == 0x0A000263
    assert lib.qos_net_ipv4_route(0x01010101, ctypes.byref(next_hop)) == 0  # 1.1.1.1
    assert next_hop.value == 0x0A000202  # gateway 10.0.2.2
    assert lib.qos_net_ipv4_route(0, ctypes.byref(next_hop)) == -1


def test_c_net_icmp_echo_gateway_and_unreachable() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_icmp_echo.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint16,
        ctypes.c_uint16,
        ctypes.c_void_p,
        ctypes.c_uint32,
        ctypes.c_void_p,
        ctypes.c_uint32,
        ctypes.c_void_p,
    ]
    lib.qos_net_icmp_echo.restype = ctypes.c_int

    lib.qos_net_reset()
    payload = (ctypes.c_ubyte * 3)(ord("q"), ord("o"), ord("s"))
    reply = (ctypes.c_ubyte * 8)()
    src_ip = ctypes.c_uint32(0)

    got = lib.qos_net_icmp_echo(0x0A000202, 1, 1, payload, 3, reply, 8, ctypes.byref(src_ip))
    assert got == 3
    assert list(reply)[:3] == [ord("q"), ord("o"), ord("s")]
    assert src_ip.value == 0x0A000202

    assert lib.qos_net_icmp_echo(0x0A000263, 1, 1, payload, 3, reply, 8, ctypes.byref(src_ip)) == -1


def test_c_net_udp_demux_by_port() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_udp_bind.argtypes = [ctypes.c_uint16]
    lib.qos_net_udp_bind.restype = ctypes.c_int
    lib.qos_net_udp_send.argtypes = [
        ctypes.c_uint16,
        ctypes.c_uint32,
        ctypes.c_uint16,
        ctypes.c_void_p,
        ctypes.c_uint32,
    ]
    lib.qos_net_udp_send.restype = ctypes.c_int
    lib.qos_net_udp_recv.argtypes = [
        ctypes.c_uint16,
        ctypes.c_void_p,
        ctypes.c_uint32,
        ctypes.c_void_p,
        ctypes.c_void_p,
    ]
    lib.qos_net_udp_recv.restype = ctypes.c_int

    lib.qos_net_reset()
    assert lib.qos_net_udp_bind(8080) == 0
    assert lib.qos_net_udp_bind(8080) == -1

    payload = (ctypes.c_ubyte * 3)(1, 2, 3)
    assert lib.qos_net_udp_send(5353, 0x0A00020F, 8080, payload, 3) == 3

    out = (ctypes.c_ubyte * 8)()
    src_ip = ctypes.c_uint32(0)
    src_port = ctypes.c_uint16(0)
    got = lib.qos_net_udp_recv(8080, out, 8, ctypes.byref(src_ip), ctypes.byref(src_port))
    assert got == 3
    assert list(out)[:3] == [1, 2, 3]
    assert src_ip.value == 0x0A00020F
    assert src_port.value == 5353
    assert lib.qos_net_udp_recv(8080, out, 8, ctypes.byref(src_ip), ctypes.byref(src_port)) == -1


def test_c_net_udp_send_to_unbound_port_behaves_as_external_egress() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_udp_bind.argtypes = [ctypes.c_uint16]
    lib.qos_net_udp_bind.restype = ctypes.c_int
    lib.qos_net_udp_send.argtypes = [
        ctypes.c_uint16,
        ctypes.c_uint32,
        ctypes.c_uint16,
        ctypes.c_void_p,
        ctypes.c_uint32,
    ]
    lib.qos_net_udp_send.restype = ctypes.c_int
    lib.qos_net_udp_recv.argtypes = [
        ctypes.c_uint16,
        ctypes.c_void_p,
        ctypes.c_uint32,
        ctypes.c_void_p,
        ctypes.c_void_p,
    ]
    lib.qos_net_udp_recv.restype = ctypes.c_int

    lib.qos_net_reset()
    assert lib.qos_net_udp_bind(8080) == 0
    payload = (ctypes.c_ubyte * 3)(9, 9, 9)
    # Destination port 8099 is unbound in this stack; send should still succeed for external egress.
    assert lib.qos_net_udp_send(5353, 0x0A00020F, 8099, payload, 3) == 3

    out = (ctypes.c_ubyte * 8)()
    src_ip = ctypes.c_uint32(0)
    src_port = ctypes.c_uint16(0)
    assert lib.qos_net_udp_recv(8099, out, 8, ctypes.byref(src_ip), ctypes.byref(src_port)) == -1


def test_c_net_rejects_low_ports_for_dynamic_use() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_udp_bind.argtypes = [ctypes.c_uint16]
    lib.qos_net_udp_bind.restype = ctypes.c_int
    lib.qos_net_udp_send.argtypes = [
        ctypes.c_uint16,
        ctypes.c_uint32,
        ctypes.c_uint16,
        ctypes.c_void_p,
        ctypes.c_uint32,
    ]
    lib.qos_net_udp_send.restype = ctypes.c_int
    lib.qos_net_tcp_listen.argtypes = [ctypes.c_uint16]
    lib.qos_net_tcp_listen.restype = ctypes.c_int
    lib.qos_net_tcp_connect.argtypes = [ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint16]
    lib.qos_net_tcp_connect.restype = ctypes.c_int

    lib.qos_net_reset()
    assert lib.qos_net_udp_bind(53) == -1
    assert lib.qos_net_udp_bind(8081) == 0
    payload = (ctypes.c_ubyte * 1)(7)
    assert lib.qos_net_udp_send(53, 0x0A00020F, 8081, payload, 1) == -1

    assert lib.qos_net_tcp_listen(80) == -1
    assert lib.qos_net_tcp_listen(8080) >= 0
    assert lib.qos_net_tcp_connect(80, 0x0A000202, 8080) == -1


def test_c_net_tcp_state_machine() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_tcp_listen.argtypes = [ctypes.c_uint16]
    lib.qos_net_tcp_listen.restype = ctypes.c_int
    lib.qos_net_tcp_connect.argtypes = [ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint16]
    lib.qos_net_tcp_connect.restype = ctypes.c_int
    lib.qos_net_tcp_step.argtypes = [ctypes.c_int, ctypes.c_uint32]
    lib.qos_net_tcp_step.restype = ctypes.c_int
    lib.qos_net_tcp_state.argtypes = [ctypes.c_int]
    lib.qos_net_tcp_state.restype = ctypes.c_int

    TCP_SYN_SENT = 2
    TCP_ESTABLISHED = 4
    TCP_FIN_WAIT_1 = 5
    TCP_FIN_WAIT_2 = 6
    TCP_TIME_WAIT = 10
    TCP_CLOSED = 0
    EVT_RX_SYN_ACK = 2
    EVT_APP_CLOSE = 4
    EVT_RX_ACK = 3
    EVT_RX_FIN = 5
    EVT_TIMEOUT = 6

    lib.qos_net_reset()
    assert lib.qos_net_tcp_listen(8080) >= 0
    conn = lib.qos_net_tcp_connect(50000, 0x0A000202, 8080)
    assert conn >= 0
    assert lib.qos_net_tcp_state(conn) == TCP_SYN_SENT
    assert lib.qos_net_tcp_step(conn, EVT_RX_SYN_ACK) == 0
    assert lib.qos_net_tcp_state(conn) == TCP_ESTABLISHED
    assert lib.qos_net_tcp_step(conn, EVT_APP_CLOSE) == 0
    assert lib.qos_net_tcp_state(conn) == TCP_FIN_WAIT_1
    assert lib.qos_net_tcp_step(conn, EVT_RX_ACK) == 0
    assert lib.qos_net_tcp_state(conn) == TCP_FIN_WAIT_2
    assert lib.qos_net_tcp_step(conn, EVT_RX_FIN) == 0
    assert lib.qos_net_tcp_state(conn) == TCP_TIME_WAIT
    assert lib.qos_net_tcp_step(conn, EVT_TIMEOUT) == 0
    assert lib.qos_net_tcp_state(conn) == TCP_CLOSED


def test_c_net_tcp_timeout_backoff_and_retry_limit() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_reset.argtypes = []
    lib.qos_net_reset.restype = None
    lib.qos_net_tcp_listen.argtypes = [ctypes.c_uint16]
    lib.qos_net_tcp_listen.restype = ctypes.c_int
    lib.qos_net_tcp_connect.argtypes = [ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint16]
    lib.qos_net_tcp_connect.restype = ctypes.c_int
    lib.qos_net_tcp_step.argtypes = [ctypes.c_int, ctypes.c_uint32]
    lib.qos_net_tcp_step.restype = ctypes.c_int
    lib.qos_net_tcp_state.argtypes = [ctypes.c_int]
    lib.qos_net_tcp_state.restype = ctypes.c_int
    lib.qos_net_tcp_rto.argtypes = [ctypes.c_int]
    lib.qos_net_tcp_rto.restype = ctypes.c_int
    lib.qos_net_tcp_retries.argtypes = [ctypes.c_int]
    lib.qos_net_tcp_retries.restype = ctypes.c_int

    TCP_SYN_SENT = 2
    TCP_CLOSED = 0
    EVT_TIMEOUT = 6

    lib.qos_net_reset()
    assert lib.qos_net_tcp_listen(8080) >= 0
    conn = lib.qos_net_tcp_connect(50000, 0x0A000202, 8080)
    assert conn >= 0
    assert lib.qos_net_tcp_state(conn) == TCP_SYN_SENT
    assert lib.qos_net_tcp_rto(conn) == 500
    assert lib.qos_net_tcp_retries(conn) == 0

    assert lib.qos_net_tcp_step(conn, EVT_TIMEOUT) == 0
    assert lib.qos_net_tcp_state(conn) == TCP_SYN_SENT
    assert lib.qos_net_tcp_rto(conn) == 1000
    assert lib.qos_net_tcp_retries(conn) == 1

    assert lib.qos_net_tcp_step(conn, EVT_TIMEOUT) == 0
    assert lib.qos_net_tcp_rto(conn) == 2000
    assert lib.qos_net_tcp_retries(conn) == 2

    assert lib.qos_net_tcp_step(conn, EVT_TIMEOUT) == 0
    assert lib.qos_net_tcp_rto(conn) == 4000
    assert lib.qos_net_tcp_retries(conn) == 3

    assert lib.qos_net_tcp_step(conn, EVT_TIMEOUT) == 0
    assert lib.qos_net_tcp_rto(conn) == 8000
    assert lib.qos_net_tcp_retries(conn) == 4

    assert lib.qos_net_tcp_step(conn, EVT_TIMEOUT) == 0
    assert lib.qos_net_tcp_rto(conn) == 16000
    assert lib.qos_net_tcp_retries(conn) == 5

    # After 5 retries, the next timeout closes the connection.
    assert lib.qos_net_tcp_step(conn, EVT_TIMEOUT) == 0
    assert lib.qos_net_tcp_state(conn) == TCP_CLOSED


def test_c_net_tcp_rto_helper_caps_at_60s() -> None:
    lib = ctypes.CDLL(str(_build_net_lib()))
    lib.qos_net_tcp_next_rto_ms.argtypes = [ctypes.c_uint32]
    lib.qos_net_tcp_next_rto_ms.restype = ctypes.c_uint32

    assert lib.qos_net_tcp_next_rto_ms(500) == 1000
    assert lib.qos_net_tcp_next_rto_ms(1000) == 2000
    assert lib.qos_net_tcp_next_rto_ms(30001) == 60000
    assert lib.qos_net_tcp_next_rto_ms(60000) == 60000
    assert lib.qos_net_tcp_next_rto_ms(70000) == 60000
