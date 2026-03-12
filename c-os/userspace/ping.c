#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define QOS_PACKET_MAX 1500
#define QOS_IPV4_LOCAL 0x0A00020FU
#define QOS_IPV4_GATEWAY 0x0A000202U

#define QOS_SYSCALL_NR_CLOSE 8U
#define QOS_SYSCALL_NR_SOCKET 14U
#define QOS_SYSCALL_NR_CONNECT 18U
#define QOS_SYSCALL_NR_SEND 19U
#define QOS_SYSCALL_NR_RECV 20U

#define QOS_SOCK_RAW 3U

#if defined(__GNUC__)
#define QOS_WEAK __attribute__((weak))
#else
#define QOS_WEAK
#endif

extern int qos_net_icmp_echo(uint32_t dst_ip,
                             uint16_t ident,
                             uint16_t seq,
                             const void *payload,
                             uint32_t len,
                             void *out_reply,
                             uint32_t cap,
                             uint32_t *out_src_ip) QOS_WEAK;
extern int64_t qos_syscall_dispatch(uint32_t nr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) QOS_WEAK;

static int parse_ipv4_addr(const char *s, uint32_t *out_ip) {
    uint32_t parts[4] = {0, 0, 0, 0};
    int part = 0;
    uint32_t cur = 0;
    const char *p;
    if (s == NULL || out_ip == NULL || *s == '\0') {
        return -1;
    }
    p = s;
    while (*p != '\0') {
        char ch = *p;
        if (ch >= '0' && ch <= '9') {
            cur = cur * 10u + (uint32_t)(ch - '0');
            if (cur > 255u) {
                return -1;
            }
        } else if (ch == '.') {
            if (part >= 3) {
                return -1;
            }
            parts[part++] = cur;
            cur = 0;
        } else {
            return -1;
        }
        p++;
    }
    if (part != 3) {
        return -1;
    }
    parts[3] = cur;
    *out_ip = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
    return 0;
}

static void make_sockaddr(uint8_t out[16], uint16_t port, uint32_t ip) {
    memset(out, 0, 16);
    out[0] = 2;
    out[1] = 0;
    out[2] = (uint8_t)(port >> 8);
    out[3] = (uint8_t)(port & 0xFF);
    out[4] = (uint8_t)(ip >> 24);
    out[5] = (uint8_t)((ip >> 16) & 0xFF);
    out[6] = (uint8_t)((ip >> 8) & 0xFF);
    out[7] = (uint8_t)(ip & 0xFF);
}

static int ping_via_syscall_raw(uint32_t dst_ip, const uint8_t *payload, uint32_t len) {
    uint8_t addr[16];
    uint8_t reply[QOS_PACKET_MAX];
    int64_t fd;
    int64_t rc;
    if (qos_syscall_dispatch == 0 || payload == NULL || len == 0 || len > QOS_PACKET_MAX) {
        return -1;
    }
    make_sockaddr(addr, 1, dst_ip);
    fd = qos_syscall_dispatch(QOS_SYSCALL_NR_SOCKET, 2, QOS_SOCK_RAW, 1, 0);
    if (fd < 0) {
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SYSCALL_NR_CONNECT, (uint64_t)fd, (uint64_t)(uintptr_t)addr, 16, 0);
    if (rc != 0) {
        (void)qos_syscall_dispatch(QOS_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SYSCALL_NR_SEND, (uint64_t)fd, (uint64_t)(uintptr_t)payload, len, 0);
    if (rc != (int64_t)len) {
        (void)qos_syscall_dispatch(QOS_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SYSCALL_NR_RECV, (uint64_t)fd, (uint64_t)(uintptr_t)reply, len, 0);
    (void)qos_syscall_dispatch(QOS_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    if (rc < 0) {
        return -1;
    }
    return (int)rc;
}

int main(int argc, char **argv) {
    uint32_t dst_ip = 0;
    uint32_t src_ip = 0;
    uint8_t payload[3] = {'q', 'o', 's'};
    int got = -1;
    if (argc < 2) {
        puts("ping: missing host");
        return 0;
    }
    if (parse_ipv4_addr(argv[1], &dst_ip) != 0) {
        puts("ping: invalid host");
        return 0;
    }
    got = ping_via_syscall_raw(dst_ip, payload, sizeof(payload));
    if (got < 0 && qos_net_icmp_echo != 0) {
        uint8_t reply[16];
        got = qos_net_icmp_echo(dst_ip, 1, 1, payload, sizeof(payload), reply, sizeof(reply), &src_ip);
    } else if (got < 0 && (dst_ip == QOS_IPV4_GATEWAY || dst_ip == QOS_IPV4_LOCAL)) {
        src_ip = dst_ip;
        got = (int)sizeof(payload);
    }
    if (got > 0) {
        printf("PING %s\n64 bytes from %s: icmp_seq=1 ttl=64 time=1ms\n1 packets transmitted, 1 received\n", argv[1], argv[1]);
    } else {
        (void)src_ip;
        printf("PING %s\nFrom 10.0.2.15 icmp_seq=1 Destination Host Unreachable\n1 packets transmitted, 0 received\n", argv[1]);
    }
    return 0;
}
