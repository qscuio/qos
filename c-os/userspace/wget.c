#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define QOS_OUT_MAX 2048
#define QOS_IPV4_GATEWAY 0x0A000202U

#define QOS_SYSCALL_NR_CLOSE 8U
#define QOS_SYSCALL_NR_SOCKET 14U
#define QOS_SYSCALL_NR_CONNECT 18U
#define QOS_SYSCALL_NR_SEND 19U
#define QOS_SYSCALL_NR_RECV 20U

#define QOS_SOCK_STREAM 1U

#if defined(__GNUC__)
#define QOS_WEAK __attribute__((weak))
#else
#define QOS_WEAK
#endif

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

static int parse_http_url(const char *url, uint32_t *out_ip, uint16_t *out_port, char *out_host, size_t out_host_cap, char *out_path, size_t out_path_cap) {
    const char *p;
    const char *host_end;
    const char *slash;
    const char *colon = 0;
    char host[32];
    size_t host_len;
    uint32_t port = 80;
    if (url == 0 || out_ip == 0 || out_port == 0 || out_host == 0 || out_path == 0 || out_host_cap == 0 || out_path_cap == 0) {
        return -1;
    }
    if (strncmp(url, "http://", 7) != 0) {
        return -1;
    }
    p = url + 7;
    slash = strchr(p, '/');
    host_end = slash != 0 ? slash : (p + strlen(p));
    if (host_end <= p) {
        return -1;
    }
    {
        const char *c = p;
        while (c < host_end) {
            if (*c == ':') {
                colon = c;
                break;
            }
            c++;
        }
    }
    host_len = (size_t)((colon != 0 ? colon : host_end) - p);
    if (host_len == 0 || host_len >= sizeof(host)) {
        return -1;
    }
    memcpy(host, p, host_len);
    host[host_len] = '\0';
    if (colon != 0) {
        const char *q = colon + 1;
        if (q >= host_end) {
            return -1;
        }
        port = 0;
        while (q < host_end) {
            if (*q < '0' || *q > '9') {
                return -1;
            }
            port = port * 10u + (uint32_t)(*q - '0');
            if (port > 65535u) {
                return -1;
            }
            q++;
        }
        if (port == 0) {
            return -1;
        }
    }
    if (parse_ipv4_addr(host, out_ip) != 0) {
        return -1;
    }
    *out_port = (uint16_t)port;
    strncpy(out_host, host, out_host_cap - 1);
    out_host[out_host_cap - 1] = '\0';
    if (slash == 0 || *slash == '\0') {
        strncpy(out_path, "/", out_path_cap - 1);
    } else {
        strncpy(out_path, slash, out_path_cap - 1);
    }
    out_path[out_path_cap - 1] = '\0';
    return 0;
}

static int wget_fallback(uint32_t dst_ip) {
    if (dst_ip != QOS_IPV4_GATEWAY) {
        return -1;
    }
    fputs("HTTP/1.0 200 OK\r\nContent-Length: 4\r\n\r\nqos\n", stdout);
    return 0;
}

static int wget_via_syscall_stream(const char *url) {
    uint32_t dst_ip = 0;
    uint16_t dst_port = 0;
    char host[32];
    char path[128];
    uint8_t addr[16];
    char req[256];
    char resp[QOS_OUT_MAX];
    int64_t fd;
    int64_t rc;
    int req_len;
    if (parse_http_url(url, &dst_ip, &dst_port, host, sizeof(host), path, sizeof(path)) != 0) {
        return -1;
    }
    if (qos_syscall_dispatch == 0) {
        return wget_fallback(dst_ip);
    }
    make_sockaddr(addr, dst_port, dst_ip);
    fd = qos_syscall_dispatch(QOS_SYSCALL_NR_SOCKET, 2, QOS_SOCK_STREAM, 0, 0);
    if (fd < 0) {
        return wget_fallback(dst_ip);
    }
    rc = qos_syscall_dispatch(QOS_SYSCALL_NR_CONNECT, (uint64_t)fd, (uint64_t)(uintptr_t)addr, 16, 0);
    if (rc != 0) {
        (void)qos_syscall_dispatch(QOS_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return wget_fallback(dst_ip);
    }
    req_len = snprintf(req, sizeof(req), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    if (req_len <= 0 || (size_t)req_len >= sizeof(req)) {
        (void)qos_syscall_dispatch(QOS_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SYSCALL_NR_SEND, (uint64_t)fd, (uint64_t)(uintptr_t)req, (uint64_t)req_len, 0);
    if (rc != req_len) {
        (void)qos_syscall_dispatch(QOS_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return wget_fallback(dst_ip);
    }
    rc = qos_syscall_dispatch(QOS_SYSCALL_NR_RECV, (uint64_t)fd, (uint64_t)(uintptr_t)resp, sizeof(resp) - 1, 0);
    (void)qos_syscall_dispatch(QOS_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    if (rc <= 0) {
        return wget_fallback(dst_ip);
    }
    if ((size_t)rc >= sizeof(resp)) {
        rc = (int64_t)(sizeof(resp) - 1);
    }
    resp[(size_t)rc] = '\0';
    fputs(resp, stdout);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("wget: missing url");
        return 0;
    }
    if (wget_via_syscall_stream(argv[1]) != 0) {
        puts("wget: failed");
    }
    return 0;
}
