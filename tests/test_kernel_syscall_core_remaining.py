from __future__ import annotations

import ctypes
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

QOS_BOOT_MAGIC = 0x514F53424F4F5400
QOS_PAGE_SIZE = 4096

SYSCALL_NR_OPEN = 5
SYSCALL_NR_READ = 6
SYSCALL_NR_WRITE = 7
SYSCALL_NR_CLOSE = 8
SYSCALL_NR_DUP2 = 9
SYSCALL_NR_MMAP = 10
SYSCALL_NR_MUNMAP = 11
SYSCALL_NR_YIELD = 12
SYSCALL_NR_SLEEP = 13
SYSCALL_NR_SOCKET = 14
SYSCALL_NR_BIND = 15
SYSCALL_NR_LISTEN = 16
SYSCALL_NR_ACCEPT = 17
SYSCALL_NR_CONNECT = 18
SYSCALL_NR_SEND = 19
SYSCALL_NR_RECV = 20
SYSCALL_NR_STAT = 21
SYSCALL_NR_GETDENTS = 24
SYSCALL_NR_LSEEK = 25
SYSCALL_NR_PIPE = 26
SYSCALL_NR_MKDIR = 22
SOCK_STREAM = 1
SOCK_DGRAM = 2
SOCK_RAW = 3


class MmapEntry(ctypes.Structure):
    _fields_ = [
        ("base", ctypes.c_uint64),
        ("length", ctypes.c_uint64),
        ("type", ctypes.c_uint32),
        ("_pad", ctypes.c_uint32),
    ]


class BootInfo(ctypes.Structure):
    _fields_ = [
        ("magic", ctypes.c_uint64),
        ("mmap_entry_count", ctypes.c_uint32),
        ("_pad0", ctypes.c_uint32),
        ("mmap_entries", MmapEntry * 128),
        ("fb_addr", ctypes.c_uint64),
        ("fb_width", ctypes.c_uint32),
        ("fb_height", ctypes.c_uint32),
        ("fb_pitch", ctypes.c_uint32),
        ("fb_bpp", ctypes.c_uint8),
        ("_pad1", ctypes.c_uint8 * 3),
        ("initramfs_addr", ctypes.c_uint64),
        ("initramfs_size", ctypes.c_uint64),
        ("acpi_rsdp_addr", ctypes.c_uint64),
        ("dtb_addr", ctypes.c_uint64),
    ]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _build_kernel_lib() -> Path:
    out = ROOT / "c-os/build/libqos_c_kernel_syscall_core_remaining.so"
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
            str(ROOT / "c-os/kernel/kernel.c"),
            str(ROOT / "c-os/kernel/init_state.c"),
            str(ROOT / "c-os/kernel/mm/mm.c"),
            str(ROOT / "c-os/kernel/sched/sched.c"),
            str(ROOT / "c-os/kernel/fs/vfs.c"),
            str(ROOT / "c-os/kernel/net/net.c"),
            str(ROOT / "c-os/kernel/drivers/drivers.c"),
            str(ROOT / "c-os/kernel/syscall/syscall.c"),
            str(ROOT / "c-os/kernel/proc/proc.c"),
        ]
    )
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}\nstdout:\n{build.stdout}"
    return out


def _valid_boot_info() -> BootInfo:
    info = BootInfo()
    info.magic = QOS_BOOT_MAGIC
    info.mmap_entry_count = 1
    info.mmap_entries[0].base = 0x100000
    info.mmap_entries[0].length = 0x200000
    info.mmap_entries[0].type = 1
    info.initramfs_addr = 0x400000
    info.initramfs_size = 0x1000
    return info


def test_c_syscall_fd_pipe_io_bridge() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0

    path = ctypes.create_string_buffer(b"/tmp-file")
    path_ptr = ctypes.c_uint64(ctypes.addressof(path)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MKDIR, path_ptr, 0, 0, 0) == 0
    stat_out = (ctypes.c_uint64 * 2)(0xDEADBEEF, 0xDEADBEEF)
    stat_ptr = ctypes.c_uint64(ctypes.addressof(stat_out)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_STAT, path_ptr, stat_ptr, 0, 0) == 0
    assert stat_out[0] == len(b"/tmp-file")

    fd = lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, path_ptr, 0, 0, 0)
    assert fd >= 0

    payload = ctypes.create_string_buffer(b"abc")
    payload_ptr = ctypes.c_uint64(ctypes.addressof(payload)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_WRITE, fd, payload_ptr, 3, 0) == 3

    assert lib.qos_syscall_dispatch(SYSCALL_NR_LSEEK, fd, 0, 0, 0) == 0

    out = ctypes.create_string_buffer(16)
    out_ptr = ctypes.c_uint64(ctypes.addressof(out)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_READ, fd, out_ptr, 16, 0) == 3
    assert bytes(out.raw[:3]) == b"abc"

    max_i64 = (1 << 63) - 1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LSEEK, fd, max_i64, 0, 0) == max_i64
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LSEEK, fd, 1, 1, 0) == -1

    assert lib.qos_syscall_dispatch(SYSCALL_NR_GETDENTS, fd, out_ptr, 16, 0) == 1
    dents_count = ctypes.c_uint32.from_buffer(out, 0)
    assert dents_count.value == 1

    dupfd = lib.qos_syscall_dispatch(SYSCALL_NR_DUP2, fd, 40, 0, 0)
    assert dupfd == 40
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, dupfd, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd, 0, 0, 0) == -1

    fd2 = lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, path_ptr, 0, 0, 0)
    assert fd2 >= 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LSEEK, fd2, 0, 0, 0) == 0
    out2 = ctypes.create_string_buffer(16)
    out2_ptr = ctypes.c_uint64(ctypes.addressof(out2)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_READ, fd2, out2_ptr, 16, 0) == 3
    assert bytes(out2.raw[:3]) == b"abc"
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, fd2, 0, 0, 0) == 0

    fds = (ctypes.c_uint32 * 2)()
    fds_ptr = ctypes.c_uint64(ctypes.addressof(fds)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_PIPE, fds_ptr, 0, 0, 0) == 0

    rfd = int(fds[0])
    wfd = int(fds[1])
    assert rfd >= 0
    assert wfd >= 0

    ping = ctypes.create_string_buffer(b"ping")
    ping_ptr = ctypes.c_uint64(ctypes.addressof(ping)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_WRITE, wfd, ping_ptr, 4, 0) == 4

    rx = ctypes.create_string_buffer(8)
    rx_ptr = ctypes.c_uint64(ctypes.addressof(rx)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_READ, rfd, rx_ptr, 8, 0) == 4
    assert bytes(rx.raw[:4]) == b"ping"

    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, rfd, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, wfd, 0, 0, 0) == 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_OPEN, 0, 0, 0, 0) == -1


def test_c_syscall_mmap_sched_socket_bridge() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_sched_add_task.argtypes = [ctypes.c_uint32]
    lib.qos_sched_add_task.restype = ctypes.c_int
    lib.qos_vmm_translate.argtypes = [ctypes.c_uint64]
    lib.qos_vmm_translate.restype = ctypes.c_uint64
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0

    va = 0x400000
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MMAP, va, QOS_PAGE_SIZE, 1, 0) == va
    assert lib.qos_vmm_translate(va) == va
    assert lib.qos_syscall_dispatch(SYSCALL_NR_MUNMAP, va, QOS_PAGE_SIZE, 0, 0) == 0
    assert lib.qos_vmm_translate(va) == 0

    assert lib.qos_sched_add_task(11) == 0
    assert lib.qos_sched_add_task(22) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_YIELD, 0, 0, 0, 0) in (11, 22)
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SLEEP, 25, 0, 0, 0) == 0

    addr = (ctypes.c_ubyte * 16)()
    addr[2] = 0x1F
    addr[3] = 0x90  # 8080
    addr[4] = 0x0A
    addr[5] = 0x00
    addr[6] = 0x02
    addr[7] = 0x0F  # 10.0.2.15
    addr_ptr = ctypes.c_uint64(ctypes.addressof(addr)).value
    addr_len = ctypes.c_uint32(16)
    addr_len_ptr = ctypes.c_uint64(ctypes.addressof(addr_len)).value

    server = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    client = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    raw = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    raw_icmp = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_RAW, 1, 0)
    raw_fail = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_RAW, 1, 0)
    raw_sendto = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_RAW, 1, 0)
    host_stream = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    no_listener = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    assert server >= 0
    assert client >= 0
    assert raw >= 0
    assert raw_icmp >= 0
    assert raw_fail >= 0
    assert raw_sendto >= 0
    assert host_stream >= 0
    assert no_listener >= 0

    bad_pkt = ctypes.create_string_buffer(b"bad")
    bad_pkt_ptr = ctypes.c_uint64(ctypes.addressof(bad_pkt)).value
    bad_out = ctypes.create_string_buffer(8)
    bad_out_ptr = ctypes.c_uint64(ctypes.addressof(bad_out)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, raw, bad_pkt_ptr, 3, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, raw, bad_out_ptr, 8, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, no_listener, addr_ptr, 16, 0) == -1

    host_addr = (ctypes.c_ubyte * 16)()
    host_addr[2] = 0x1F
    host_addr[3] = 0x90  # 8080
    host_addr[4] = 0x0A
    host_addr[5] = 0x00
    host_addr[6] = 0x02
    host_addr[7] = 0x02  # 10.0.2.2 gateway
    host_addr_ptr = ctypes.c_uint64(ctypes.addressof(host_addr)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, host_stream, host_addr_ptr, 16, 0) == 0
    http_req = ctypes.create_string_buffer(b"GET / HTTP/1.0\r\nHost: 10.0.2.2\r\n\r\n")
    http_req_ptr = ctypes.c_uint64(ctypes.addressof(http_req)).value
    assert (
        lib.qos_syscall_dispatch(SYSCALL_NR_SEND, host_stream, http_req_ptr, len(http_req.value), 0) == len(http_req.value)
    )
    http_resp = ctypes.create_string_buffer(256)
    http_resp_ptr = ctypes.c_uint64(ctypes.addressof(http_resp)).value
    got_http = lib.qos_syscall_dispatch(SYSCALL_NR_RECV, host_stream, http_resp_ptr, 256, 0)
    assert got_http > 0
    assert b"HTTP/1.0 200 OK" in bytes(http_resp.raw[:got_http])

    icmp_addr = (ctypes.c_ubyte * 16)()
    icmp_addr[2] = 0x00
    icmp_addr[3] = 0x01
    icmp_addr[4] = 0x0A
    icmp_addr[5] = 0x00
    icmp_addr[6] = 0x02
    icmp_addr[7] = 0x02  # 10.0.2.2 gateway
    icmp_addr_ptr = ctypes.c_uint64(ctypes.addressof(icmp_addr)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, raw_icmp, icmp_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, raw_icmp, bad_pkt_ptr, 3, 0) == 3
    src_addr = (ctypes.c_ubyte * 16)()
    src_addr_ptr = ctypes.c_uint64(ctypes.addressof(src_addr)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, raw_icmp, bad_out_ptr, 8, src_addr_ptr) == 3
    assert bytes(bad_out.raw[:3]) == b"bad"
    src_ip = (int(src_addr[4]) << 24) | (int(src_addr[5]) << 16) | (int(src_addr[6]) << 8) | int(src_addr[7])
    assert src_ip == 0x0A000202

    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, raw_sendto, bad_pkt_ptr, 3, icmp_addr_ptr) == 3
    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, raw_sendto, bad_out_ptr, 8, src_addr_ptr) == 3
    assert bytes(bad_out.raw[:3]) == b"bad"

    icmp_fail_addr = (ctypes.c_ubyte * 16)()
    icmp_fail_addr[2] = 0x00
    icmp_fail_addr[3] = 0x01
    icmp_fail_addr[4] = 0x0A
    icmp_fail_addr[5] = 0x00
    icmp_fail_addr[6] = 0x02
    icmp_fail_addr[7] = 0x63  # 10.0.2.99
    icmp_fail_addr_ptr = ctypes.c_uint64(ctypes.addressof(icmp_fail_addr)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, raw_fail, icmp_fail_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, raw_fail, bad_pkt_ptr, 3, 0) == -1

    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, server, addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LISTEN, server, 4, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_ACCEPT, server, addr_ptr, addr_len_ptr, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, client, addr_ptr, 16, 0) == 0

    accepted = lib.qos_syscall_dispatch(SYSCALL_NR_ACCEPT, server, addr_ptr, addr_len_ptr, 0)
    assert accepted >= 0
    assert addr_len.value == 16
    peer_port = (int(addr[2]) << 8) | int(addr[3])
    peer_ip = (int(addr[4]) << 24) | (int(addr[5]) << 16) | (int(addr[6]) << 8) | int(addr[7])
    assert peer_port >= 1024
    assert peer_ip == 0x0A00020F  # 10.0.2.15

    packet = ctypes.create_string_buffer(b"net")
    packet_ptr = ctypes.c_uint64(ctypes.addressof(packet)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, client, packet_ptr, 3, 0) == 3

    recv_buf = ctypes.create_string_buffer(16)
    recv_ptr = ctypes.c_uint64(ctypes.addressof(recv_buf)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, accepted, recv_ptr, 16, 0) == 3
    assert bytes(recv_buf.raw[:3]) == b"net"

    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, accepted, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, client, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, server, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, raw, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, raw_icmp, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, raw_fail, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, raw_sendto, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, host_stream, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, no_listener, 0, 0, 0) == 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, accepted, recv_ptr, 16, 0) == -1

    udp_a = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0)
    udp_b = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0)
    udp_c = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0)
    udp_raw = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0)
    udp_any = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0)
    assert udp_a >= 0
    assert udp_b >= 0
    assert udp_c >= 0
    assert udp_raw >= 0
    assert udp_any >= 0

    a_addr = (ctypes.c_ubyte * 16)()
    a_addr[2] = 0x0F
    a_addr[3] = 0xA1  # 4001
    a_addr[4] = 0x0A
    a_addr[5] = 0x00
    a_addr[6] = 0x02
    a_addr[7] = 0x0B  # 10.0.2.11
    b_addr = (ctypes.c_ubyte * 16)()
    b_addr[2] = 0x0F
    b_addr[3] = 0xA2  # 4002
    b_addr[4] = 0x0A
    b_addr[5] = 0x00
    b_addr[6] = 0x02
    b_addr[7] = 0x0C  # 10.0.2.12
    c_addr = (ctypes.c_ubyte * 16)()
    c_addr[2] = 0x0F
    c_addr[3] = 0xA3  # 4003
    c_addr[4] = 0x0A
    c_addr[5] = 0x00
    c_addr[6] = 0x02
    c_addr[7] = 0x0D  # 10.0.2.13
    a_addr_ptr = ctypes.c_uint64(ctypes.addressof(a_addr)).value
    b_addr_ptr = ctypes.c_uint64(ctypes.addressof(b_addr)).value
    c_addr_ptr = ctypes.c_uint64(ctypes.addressof(c_addr)).value
    any_addr = (ctypes.c_ubyte * 16)()
    any_addr[2] = 0x0F
    any_addr[3] = 0xA4  # 4004
    any_addr[4] = 0x00
    any_addr[5] = 0x00
    any_addr[6] = 0x00
    any_addr[7] = 0x00  # INADDR_ANY
    any_addr_ptr = ctypes.c_uint64(ctypes.addressof(any_addr)).value

    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, udp_a, a_addr_ptr, 4, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, udp_a, a_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LISTEN, udp_a, 1, 0, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, udp_b, b_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, udp_c, c_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, udp_any, any_addr_ptr, 16, 0) == 0

    src_addr_udp = (ctypes.c_ubyte * 16)()
    src_addr_udp_ptr = ctypes.c_uint64(ctypes.addressof(src_addr_udp)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, udp_a, packet_ptr, 3, b_addr_ptr) == 3
    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, udp_b, recv_ptr, 16, src_addr_udp_ptr) == 3
    assert bytes(recv_buf.raw[:3]) == b"net"
    udp_src_port = (int(src_addr_udp[2]) << 8) | int(src_addr_udp[3])
    udp_src_ip = (int(src_addr_udp[4]) << 24) | (int(src_addr_udp[5]) << 16) | (int(src_addr_udp[6]) << 8) | int(
        src_addr_udp[7]
    )
    assert udp_src_port == 4001
    assert udp_src_ip == 0x0A00020F

    # No TCP listener exists now; UDP connect must still work.
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, udp_a, b_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, udp_b, a_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, udp_c, a_addr_ptr, 16, 0) == 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, udp_raw, packet_ptr, 3, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, udp_a, packet_ptr, 3, 0) == 3
    ext_addr = (ctypes.c_ubyte * 16)()
    ext_addr[2] = 0x1F
    ext_addr[3] = 0x90  # 8080
    ext_addr[4] = 0x0A
    ext_addr[5] = 0x00
    ext_addr[6] = 0x02
    ext_addr[7] = 0x63  # 10.0.2.99 external/unbound
    ext_addr_ptr = ctypes.c_uint64(ctypes.addressof(ext_addr)).value
    assert lib.qos_syscall_dispatch(SYSCALL_NR_SEND, udp_raw, packet_ptr, 3, ext_addr_ptr) == 3
    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, udp_c, recv_ptr, 16, 0) == -1
    assert lib.qos_syscall_dispatch(SYSCALL_NR_RECV, udp_b, recv_ptr, 16, 0) == 3
    assert bytes(recv_buf.raw[:3]) == b"net"

    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, udp_a, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, udp_b, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, udp_c, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, udp_raw, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, udp_any, 0, 0, 0) == 0


def test_c_socket_ports_reusable_after_close() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0

    udp_addr = (ctypes.c_ubyte * 16)()
    udp_addr[2] = 0x0F
    udp_addr[3] = 0xA1  # 4001
    udp_addr[4] = 0x0A
    udp_addr[5] = 0x00
    udp_addr[6] = 0x02
    udp_addr[7] = 0x0B  # 10.0.2.11
    udp_addr_ptr = ctypes.c_uint64(ctypes.addressof(udp_addr)).value

    u1 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0)
    assert u1 >= 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, u1, udp_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, u1, 0, 0, 0) == 0

    u2 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_DGRAM, 0, 0)
    assert u2 >= 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, u2, udp_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, u2, 0, 0, 0) == 0

    tcp_addr = (ctypes.c_ubyte * 16)()
    tcp_addr[2] = 0x1F
    tcp_addr[3] = 0x90  # 8080
    tcp_addr[4] = 0x0A
    tcp_addr[5] = 0x00
    tcp_addr[6] = 0x02
    tcp_addr[7] = 0x0F  # 10.0.2.15
    tcp_addr_ptr = ctypes.c_uint64(ctypes.addressof(tcp_addr)).value

    s1 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    assert s1 >= 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, s1, tcp_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LISTEN, s1, 1, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, s1, 0, 0, 0) == 0

    s2 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    assert s2 >= 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, s2, tcp_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LISTEN, s2, 1, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, s2, 0, 0, 0) == 0


def test_c_accept_handles_multiple_pending_peers_fifo() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0

    srv_addr = (ctypes.c_ubyte * 16)()
    srv_addr[2] = 0x1F
    srv_addr[3] = 0x91  # 8081
    srv_addr[4] = 0x0A
    srv_addr[5] = 0x00
    srv_addr[6] = 0x02
    srv_addr[7] = 0x0F
    srv_addr_ptr = ctypes.c_uint64(ctypes.addressof(srv_addr)).value

    server = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    c1 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    c2 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    assert server >= 0 and c1 >= 0 and c2 >= 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, server, srv_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LISTEN, server, 4, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, c1, srv_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, c2, srv_addr_ptr, 16, 0) == 0

    a1 = (ctypes.c_ubyte * 16)()
    a2 = (ctypes.c_ubyte * 16)()
    a1_len = ctypes.c_uint32(16)
    a2_len = ctypes.c_uint32(16)
    a1_ptr = ctypes.c_uint64(ctypes.addressof(a1)).value
    a2_ptr = ctypes.c_uint64(ctypes.addressof(a2)).value
    a1_len_ptr = ctypes.c_uint64(ctypes.addressof(a1_len)).value
    a2_len_ptr = ctypes.c_uint64(ctypes.addressof(a2_len)).value

    s1 = lib.qos_syscall_dispatch(SYSCALL_NR_ACCEPT, server, a1_ptr, a1_len_ptr, 0)
    s2 = lib.qos_syscall_dispatch(SYSCALL_NR_ACCEPT, server, a2_ptr, a2_len_ptr, 0)
    assert s1 >= 0 and s2 >= 0
    assert a1_len.value == 16
    assert a2_len.value == 16
    p1 = (int(a1[2]) << 8) | int(a1[3])
    p2 = (int(a2[2]) << 8) | int(a2[3])
    assert p1 >= 1024
    assert p2 >= 1024
    assert p1 != p2

    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, s1, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, s2, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, c1, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, c2, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, server, 0, 0, 0) == 0


def test_c_listen_backlog_limits_pending_connects() -> None:
    lib = ctypes.CDLL(str(_build_kernel_lib()))
    lib.qos_kernel_entry.argtypes = [ctypes.POINTER(BootInfo)]
    lib.qos_kernel_entry.restype = ctypes.c_int
    lib.qos_syscall_dispatch.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.qos_syscall_dispatch.restype = ctypes.c_int64

    info = _valid_boot_info()
    assert lib.qos_kernel_entry(ctypes.byref(info)) == 0

    srv_addr = (ctypes.c_ubyte * 16)()
    srv_addr[2] = 0x1F
    srv_addr[3] = 0x92  # 8082
    srv_addr[4] = 0x0A
    srv_addr[5] = 0x00
    srv_addr[6] = 0x02
    srv_addr[7] = 0x0F
    srv_addr_ptr = ctypes.c_uint64(ctypes.addressof(srv_addr)).value

    server = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    c1 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    c2 = lib.qos_syscall_dispatch(SYSCALL_NR_SOCKET, 2, SOCK_STREAM, 0, 0)
    assert server >= 0 and c1 >= 0 and c2 >= 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_BIND, server, srv_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_LISTEN, server, 1, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, c1, srv_addr_ptr, 16, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, c2, srv_addr_ptr, 16, 0) == -1

    out = (ctypes.c_ubyte * 16)()
    out_len = ctypes.c_uint32(16)
    out_ptr = ctypes.c_uint64(ctypes.addressof(out)).value
    out_len_ptr = ctypes.c_uint64(ctypes.addressof(out_len)).value
    accepted = lib.qos_syscall_dispatch(SYSCALL_NR_ACCEPT, server, out_ptr, out_len_ptr, 0)
    assert accepted >= 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_CONNECT, c2, srv_addr_ptr, 16, 0) == 0

    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, accepted, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, c1, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, c2, 0, 0, 0) == 0
    assert lib.qos_syscall_dispatch(SYSCALL_NR_CLOSE, server, 0, 0, 0) == 0
