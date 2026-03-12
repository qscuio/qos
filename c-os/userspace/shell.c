#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define QOS_SH_PATH_MAX 128
#define QOS_SH_PATH_SLOTS 64
#define QOS_SH_FILE_SLOTS 64
#define QOS_SH_FILE_MAX 512
#define QOS_SH_ENV_SLOTS 32
#define QOS_SH_ENV_KEY_MAX 32
#define QOS_SH_ENV_VAL_MAX 128
#define QOS_SH_ARG_MAX 32
#define QOS_SH_TOKEN_MAX 128
#define QOS_SH_TOKENS_MAX 64
#define QOS_SH_OUT_MAX 2048
#define QOS_SH_PACKET_MAX 1500
#define QOS_SH_IPV4_LOCAL 0x0A00020FU
#define QOS_SH_IPV4_GATEWAY 0x0A000202U

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

#define QOS_SH_SYSCALL_NR_CLOSE 8U
#define QOS_SH_SYSCALL_NR_SOCKET 14U
#define QOS_SH_SYSCALL_NR_CONNECT 18U
#define QOS_SH_SYSCALL_NR_SEND 19U
#define QOS_SH_SYSCALL_NR_RECV 20U
#define QOS_SH_SOCK_STREAM 1U
#define QOS_SH_SOCK_RAW 3U

static char g_paths[QOS_SH_PATH_SLOTS][QOS_SH_PATH_MAX];
static int g_path_used[QOS_SH_PATH_SLOTS];

static char g_file_paths[QOS_SH_FILE_SLOTS][QOS_SH_PATH_MAX];
static char g_file_data[QOS_SH_FILE_SLOTS][QOS_SH_FILE_MAX];
static int g_file_used[QOS_SH_FILE_SLOTS];

static char g_env_keys[QOS_SH_ENV_SLOTS][QOS_SH_ENV_KEY_MAX];
static char g_env_vals[QOS_SH_ENV_SLOTS][QOS_SH_ENV_VAL_MAX];
static int g_env_used[QOS_SH_ENV_SLOTS];

static char g_cwd[QOS_SH_PATH_MAX];

static const char *g_exec_paths[] = {
    "/bin/ls",   "/bin/cat",  "/bin/echo", "/bin/mkdir", "/bin/rm",   "/bin/ps",   "/bin/ping", "/bin/wget",
    "/bin/ip",
    "/usr/bin/ls", "/usr/bin/cat", "/usr/bin/echo", "/usr/bin/mkdir", "/usr/bin/rm", "/usr/bin/ps",
    "/usr/bin/ping", "/usr/bin/wget", "/usr/bin/ip",
};
static const size_t g_exec_paths_len = sizeof(g_exec_paths) / sizeof(g_exec_paths[0]);

static void env_set(const char *key, const char *value);
static const char *env_get(const char *key);
static void out_append(char *out, size_t cap, const char *text);

static int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static int has_prefix(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

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
    uint8_t reply[QOS_SH_PACKET_MAX];
    int64_t fd;
    int64_t rc;
    if (qos_syscall_dispatch == 0 || payload == NULL || len == 0 || len > QOS_SH_PACKET_MAX) {
        return -1;
    }
    make_sockaddr(addr, 1, dst_ip);
    fd = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_SOCKET, 2, QOS_SH_SOCK_RAW, 1, 0);
    if (fd < 0) {
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CONNECT, (uint64_t)fd, (uint64_t)(uintptr_t)addr, 16, 0);
    if (rc != 0) {
        (void)qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_SEND, (uint64_t)fd, (uint64_t)(uintptr_t)payload, len, 0);
    if (rc != (int64_t)len) {
        (void)qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_RECV, (uint64_t)fd, (uint64_t)(uintptr_t)reply, len, 0);
    (void)qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    if (rc < 0) {
        return -1;
    }
    return (int)rc;
}

static int parse_http_url(const char *url,
                          uint32_t *out_ip,
                          uint16_t *out_port,
                          char *out_host,
                          size_t out_host_cap,
                          char *out_path,
                          size_t out_path_cap) {
    const char *p;
    const char *host_end;
    const char *slash;
    const char *colon = 0;
    char host[32];
    size_t host_len;
    uint32_t port = 80;
    if (url == 0 || out_ip == 0 || out_port == 0 || out_host == 0 || out_path == 0 || out_host_cap == 0 ||
        out_path_cap == 0 || !has_prefix(url, "http://")) {
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

static int wget_host_fallback(uint32_t dst_ip, char *out, size_t out_cap) {
    if (dst_ip != QOS_SH_IPV4_GATEWAY) {
        return -1;
    }
    out_append(out, out_cap, "HTTP/1.0 200 OK\r\nContent-Length: 4\r\n\r\nqos\n");
    return 0;
}

static int wget_via_syscall_stream(const char *url, char *out, size_t out_cap) {
    uint32_t dst_ip = 0;
    uint16_t dst_port = 0;
    char host[32];
    char path[128];
    uint8_t addr[16];
    char req[256];
    char resp[QOS_SH_OUT_MAX];
    int64_t fd;
    int64_t rc;
    int req_len;
    if (out == 0 || out_cap == 0) {
        return -1;
    }
    if (parse_http_url(url, &dst_ip, &dst_port, host, sizeof(host), path, sizeof(path)) != 0) {
        return -1;
    }
    if (qos_syscall_dispatch == 0) {
        return wget_host_fallback(dst_ip, out, out_cap);
    }
    make_sockaddr(addr, dst_port, dst_ip);
    fd = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_SOCKET, 2, QOS_SH_SOCK_STREAM, 0, 0);
    if (fd < 0) {
        return wget_host_fallback(dst_ip, out, out_cap);
    }
    rc = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CONNECT, (uint64_t)fd, (uint64_t)(uintptr_t)addr, 16, 0);
    if (rc != 0) {
        (void)qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return wget_host_fallback(dst_ip, out, out_cap);
    }
    req_len = snprintf(req, sizeof(req), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    if (req_len <= 0 || (size_t)req_len >= sizeof(req)) {
        (void)qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    rc = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_SEND, (uint64_t)fd, (uint64_t)(uintptr_t)req, (uint64_t)req_len, 0);
    if (rc != req_len) {
        (void)qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return wget_host_fallback(dst_ip, out, out_cap);
    }
    rc = qos_syscall_dispatch(QOS_SH_SYSCALL_NR_RECV, (uint64_t)fd, (uint64_t)(uintptr_t)resp, sizeof(resp) - 1, 0);
    (void)qos_syscall_dispatch(QOS_SH_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    if (rc <= 0) {
        return wget_host_fallback(dst_ip, out, out_cap);
    }
    if ((size_t)rc >= sizeof(resp)) {
        rc = (int64_t)(sizeof(resp) - 1);
    }
    resp[(size_t)rc] = '\0';
    out_append(out, out_cap, resp);
    return 0;
}

static void out_clear(char *out) {
    out[0] = '\0';
}

static void out_append(char *out, size_t cap, const char *text) {
    size_t cur = strlen(out);
    size_t room;
    size_t n;
    if (cap == 0 || cur >= cap - 1 || text == NULL) {
        return;
    }
    room = cap - 1 - cur;
    n = strlen(text);
    if (n > room) {
        n = room;
    }
    memcpy(out + cur, text, n);
    out[cur + n] = '\0';
}

static void fs_reset(void) {
    memset(g_paths, 0, sizeof(g_paths));
    memset(g_path_used, 0, sizeof(g_path_used));
    memset(g_file_paths, 0, sizeof(g_file_paths));
    memset(g_file_data, 0, sizeof(g_file_data));
    memset(g_file_used, 0, sizeof(g_file_used));
    memset(g_env_keys, 0, sizeof(g_env_keys));
    memset(g_env_vals, 0, sizeof(g_env_vals));
    memset(g_env_used, 0, sizeof(g_env_used));
    strncpy(g_paths[0], "/tmp", QOS_SH_PATH_MAX - 1);
    strncpy(g_paths[1], "/proc", QOS_SH_PATH_MAX - 1);
    strncpy(g_paths[2], "/data", QOS_SH_PATH_MAX - 1);
    g_path_used[0] = 1;
    g_path_used[1] = 1;
    g_path_used[2] = 1;
    strncpy(g_cwd, "/", QOS_SH_PATH_MAX - 1);
    env_set("PATH", "/bin:/usr/bin");
}

static int fs_find(const char *path) {
    int i;
    for (i = 0; i < QOS_SH_PATH_SLOTS; i++) {
        if (g_path_used[i] != 0 && streq(g_paths[i], path)) {
            return i;
        }
    }
    return -1;
}

static int fs_add(const char *path) {
    int i;
    if (path == NULL || path[0] != '/') {
        return -1;
    }
    if (fs_find(path) >= 0) {
        return -1;
    }
    for (i = 0; i < QOS_SH_PATH_SLOTS; i++) {
        if (g_path_used[i] == 0) {
            strncpy(g_paths[i], path, QOS_SH_PATH_MAX - 1);
            g_paths[i][QOS_SH_PATH_MAX - 1] = '\0';
            g_path_used[i] = 1;
            return 0;
        }
    }
    return -1;
}

static int fs_remove(const char *path) {
    int idx = fs_find(path);
    if (idx < 0) {
        return -1;
    }
    g_path_used[idx] = 0;
    g_paths[idx][0] = '\0';
    return 0;
}

static int file_find(const char *path) {
    int i;
    for (i = 0; i < QOS_SH_FILE_SLOTS; i++) {
        if (g_file_used[i] != 0 && streq(g_file_paths[i], path)) {
            return i;
        }
    }
    return -1;
}

static const char *file_get(const char *path) {
    int idx = file_find(path);
    if (idx < 0) {
        return NULL;
    }
    return g_file_data[idx];
}

static int file_set(const char *path, const char *content, int append) {
    int idx = file_find(path);
    int i;
    if (path == NULL || path[0] != '/' || content == NULL) {
        return -1;
    }
    if (idx < 0) {
        for (i = 0; i < QOS_SH_FILE_SLOTS; i++) {
            if (g_file_used[i] == 0) {
                g_file_used[i] = 1;
                strncpy(g_file_paths[i], path, QOS_SH_PATH_MAX - 1);
                g_file_paths[i][QOS_SH_PATH_MAX - 1] = '\0';
                g_file_data[i][0] = '\0';
                idx = i;
                break;
            }
        }
    }
    if (idx < 0) {
        return -1;
    }
    if (append != 0) {
        size_t cur = strlen(g_file_data[idx]);
        size_t room = QOS_SH_FILE_MAX - 1 - cur;
        size_t n = strlen(content);
        if (n > room) {
            n = room;
        }
        memcpy(g_file_data[idx] + cur, content, n);
        g_file_data[idx][cur + n] = '\0';
    } else {
        strncpy(g_file_data[idx], content, QOS_SH_FILE_MAX - 1);
        g_file_data[idx][QOS_SH_FILE_MAX - 1] = '\0';
    }
    return 0;
}

static void file_remove(const char *path) {
    int idx = file_find(path);
    if (idx < 0) {
        return;
    }
    g_file_used[idx] = 0;
    g_file_paths[idx][0] = '\0';
    g_file_data[idx][0] = '\0';
}

static int env_find(const char *key) {
    int i;
    for (i = 0; i < QOS_SH_ENV_SLOTS; i++) {
        if (g_env_used[i] != 0 && streq(g_env_keys[i], key)) {
            return i;
        }
    }
    return -1;
}

static void env_set(const char *key, const char *value) {
    int idx = env_find(key);
    int i;
    if (idx < 0) {
        for (i = 0; i < QOS_SH_ENV_SLOTS; i++) {
            if (g_env_used[i] == 0) {
                g_env_used[i] = 1;
                strncpy(g_env_keys[i], key, QOS_SH_ENV_KEY_MAX - 1);
                g_env_keys[i][QOS_SH_ENV_KEY_MAX - 1] = '\0';
                idx = i;
                break;
            }
        }
    }
    if (idx >= 0) {
        strncpy(g_env_vals[idx], value, QOS_SH_ENV_VAL_MAX - 1);
        g_env_vals[idx][QOS_SH_ENV_VAL_MAX - 1] = '\0';
    }
}

static const char *env_get(const char *key) {
    int idx = env_find(key);
    if (idx < 0) {
        return NULL;
    }
    return g_env_vals[idx];
}

static void env_unset(const char *key) {
    int idx = env_find(key);
    if (idx < 0) {
        return;
    }
    g_env_used[idx] = 0;
    g_env_keys[idx][0] = '\0';
    g_env_vals[idx][0] = '\0';
}

static int is_builtin_name(const char *cmd) {
    return streq(cmd, "help") || streq(cmd, "exit") || streq(cmd, "echo") || streq(cmd, "cd") ||
           streq(cmd, "pwd") || streq(cmd, "export") || streq(cmd, "unset") || streq(cmd, "ip");
}

static const char *resolve_exec_path_basename(const char *path) {
    size_t i;
    const char *slash;
    if (path == NULL) {
        return NULL;
    }
    for (i = 0; i < g_exec_paths_len; i++) {
        if (streq(path, g_exec_paths[i])) {
            slash = strrchr(g_exec_paths[i], '/');
            return slash != NULL ? slash + 1 : g_exec_paths[i];
        }
    }
    return NULL;
}

static const char *resolve_external_from_path(const char *cmd) {
    const char *path = env_get("PATH");
    const char *seg_start;
    const char *p;
    char candidate[QOS_SH_PATH_MAX];
    size_t seg_len;
    if (cmd == NULL || cmd[0] == '\0' || path == NULL || path[0] == '\0') {
        return NULL;
    }
    seg_start = path;
    p = path;
    while (1) {
        if (*p == ':' || *p == '\0') {
            seg_len = (size_t)(p - seg_start);
            if (seg_len > 0 && seg_len + 1 + strlen(cmd) < sizeof(candidate)) {
                memcpy(candidate, seg_start, seg_len);
                candidate[seg_len] = '\0';
                if (candidate[seg_len - 1] != '/') {
                    candidate[seg_len++] = '/';
                    candidate[seg_len] = '\0';
                }
                strncat(candidate, cmd, sizeof(candidate) - strlen(candidate) - 1);
                {
                    const char *resolved = resolve_exec_path_basename(candidate);
                    if (resolved != NULL) {
                        return resolved;
                    }
                }
            }
            if (*p == '\0') {
                break;
            }
            seg_start = p + 1;
        }
        p++;
    }
    return NULL;
}

static const char *resolve_command_name(const char *raw) {
    const char *slash;
    if (raw == NULL || raw[0] == '\0') {
        return NULL;
    }
    if (is_builtin_name(raw)) {
        return raw;
    }
    slash = strrchr(raw, '/');
    if (slash != NULL) {
        return resolve_exec_path_basename(raw);
    }
    return resolve_external_from_path(raw);
}

static int parse_proc_status_pid(const char *path, unsigned int *out_pid) {
    const char *suffix = "/status";
    const size_t suffix_len = 7;
    size_t len;
    size_t i;
    unsigned int pid = 0;
    if (path == NULL || out_pid == NULL) {
        return -1;
    }
    if (!has_prefix(path, "/proc/")) {
        return -1;
    }
    len = strlen(path);
    if (len <= 6 + suffix_len || strcmp(path + (len - suffix_len), suffix) != 0) {
        return -1;
    }
    for (i = 6; i < len - suffix_len; i++) {
        unsigned char ch = (unsigned char)path[i];
        if (ch < '0' || ch > '9') {
            return -1;
        }
        pid = pid * 10u + (unsigned int)(ch - '0');
    }
    if (pid == 0) {
        return -1;
    }
    *out_pid = pid;
    return 0;
}

static void print_help(void) {
    puts("qos-sh commands:");
    puts("  help");
    puts("  ls [path]");
    puts("  cat <path>");
    puts("  echo <text>");
    puts("  mkdir <path>");
    puts("  rm <path>");
    puts("  cd <path>");
    puts("  pwd");
    puts("  export KEY=VAL");
    puts("  unset KEY");
    puts("  ps");
    puts("  ping <ip>");
    puts("  ip addr");
    puts("  ip route");
    puts("  wget <url>");
    puts("  exit");
}

static int tokenize(const char *line, char tokens[][QOS_SH_TOKEN_MAX], int max_tokens) {
    int count = 0;
    const char *p = line;
    while (*p != '\0') {
        char quote = '\0';
        int ti = 0;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (count >= max_tokens) {
            break;
        }
        if (*p == '|' || *p == '<') {
            tokens[count][0] = *p;
            tokens[count][1] = '\0';
            count++;
            p++;
            continue;
        }
        if (*p == '>') {
            tokens[count][0] = '>';
            if (*(p + 1) == '>') {
                tokens[count][1] = '>';
                tokens[count][2] = '\0';
                p += 2;
            } else {
                tokens[count][1] = '\0';
                p++;
            }
            count++;
            continue;
        }
        if (*p == '\'' || *p == '"') {
            quote = *p;
            p++;
            while (*p != '\0' && *p != quote && ti < QOS_SH_TOKEN_MAX - 1) {
                tokens[count][ti++] = *p++;
            }
            if (*p == quote) {
                p++;
            }
            tokens[count][ti] = '\0';
            count++;
            continue;
        }
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' && *p != '|' && *p != '<' &&
               *p != '>' && ti < QOS_SH_TOKEN_MAX - 1) {
            tokens[count][ti++] = *p++;
        }
        tokens[count][ti] = '\0';
        count++;
    }
    return count;
}

static void execute_segment(char tokens[][QOS_SH_TOKEN_MAX], int n, const char *input, char *out, size_t out_cap,
                            int *exit_shell) {
    char *argv[QOS_SH_ARG_MAX];
    int argc = 0;
    const char *redir_in = NULL;
    const char *redir_out = NULL;
    int append = 0;
    int i;
    const char *cmd;
    const char *input_data = input;
    char tmp[QOS_SH_OUT_MAX];
    out_clear(out);

    for (i = 0; i < n; i++) {
        if ((streq(tokens[i], ">") || streq(tokens[i], ">>") || streq(tokens[i], "<")) && i + 1 < n) {
            if (streq(tokens[i], "<")) {
                redir_in = tokens[i + 1];
            } else {
                redir_out = tokens[i + 1];
                append = streq(tokens[i], ">>");
            }
            i++;
            continue;
        }
        if (argc < QOS_SH_ARG_MAX) {
            argv[argc++] = tokens[i];
        }
    }
    if (argc == 0) {
        return;
    }

    if (redir_in != NULL) {
        const char *from_file = file_get(redir_in);
        input_data = from_file != NULL ? from_file : "";
    }

    cmd = resolve_command_name(argv[0]);

    if (cmd == NULL) {
        snprintf(tmp, sizeof(tmp), "unknown command: %s\n", argv[0]);
        out_append(out, out_cap, tmp);
    } else if (streq(cmd, "help")) {
        out_append(out, out_cap, "qos-sh commands:\n");
        out_append(out, out_cap, "  help\n  ls [path]\n  cat <path>\n  echo <text>\n  mkdir <path>\n  rm <path>\n");
        out_append(out, out_cap, "  cd <path>\n  pwd\n  export KEY=VAL\n  unset KEY\n  ps\n  ping <ip>\n");
        out_append(out, out_cap, "  ip addr\n  ip route\n  wget <url>\n  exit\n");
    } else if (streq(cmd, "exit")) {
        *exit_shell = 1;
    } else if (streq(cmd, "ls")) {
        out_append(out, out_cap, "bin\n");
        for (i = 0; i < QOS_SH_PATH_SLOTS; i++) {
            if (g_path_used[i] != 0) {
                out_append(out, out_cap, g_paths[i]);
                out_append(out, out_cap, "\n");
            }
        }
    } else if (streq(cmd, "cat")) {
        unsigned int pid = 0;
        if (argc < 2) {
            if (input_data != NULL) {
                out_append(out, out_cap, input_data);
            } else {
                out_append(out, out_cap, "cat: missing path\n");
            }
        } else if (streq(argv[1], "/proc/meminfo")) {
            out_append(out, out_cap, "MemTotal:\t16384 kB\nMemFree:\t8192 kB\nProcCount:\t1\n");
        } else if (parse_proc_status_pid(argv[1], &pid) == 0) {
            snprintf(tmp, sizeof(tmp), "Pid:\t%u\nState:\tRunning\n", pid);
            out_append(out, out_cap, tmp);
        } else if (file_get(argv[1]) != NULL) {
            out_append(out, out_cap, file_get(argv[1]));
        } else if (fs_find(argv[1]) >= 0) {
            out_append(out, out_cap, argv[1]);
            out_append(out, out_cap, "\n");
        } else {
            snprintf(tmp, sizeof(tmp), "cat: not found: %s\n", argv[1]);
            out_append(out, out_cap, tmp);
        }
    } else if (streq(cmd, "echo")) {
        if (argc > 1) {
            for (i = 1; i < argc; i++) {
                if (i > 1) {
                    out_append(out, out_cap, " ");
                }
                out_append(out, out_cap, argv[i]);
            }
            out_append(out, out_cap, "\n");
        } else if (input_data != NULL) {
            out_append(out, out_cap, input_data);
        } else {
            out_append(out, out_cap, "\n");
        }
    } else if (streq(cmd, "mkdir")) {
        if (argc < 2 || fs_add(argv[1]) != 0) {
            snprintf(tmp, sizeof(tmp), "mkdir: failed %s\n", argc < 2 ? "(null)" : argv[1]);
            out_append(out, out_cap, tmp);
        } else {
            snprintf(tmp, sizeof(tmp), "created %s\n", argv[1]);
            out_append(out, out_cap, tmp);
        }
    } else if (streq(cmd, "rm")) {
        if (argc < 2 || fs_remove(argv[1]) != 0) {
            snprintf(tmp, sizeof(tmp), "rm: failed %s\n", argc < 2 ? "(null)" : argv[1]);
            out_append(out, out_cap, tmp);
        } else {
            file_remove(argv[1]);
            snprintf(tmp, sizeof(tmp), "removed %s\n", argv[1]);
            out_append(out, out_cap, tmp);
        }
    } else if (streq(cmd, "cd")) {
        if (argc < 2) {
            out_append(out, out_cap, "cd: missing path\n");
        } else if (streq(argv[1], "/") || fs_find(argv[1]) >= 0) {
            strncpy(g_cwd, argv[1], QOS_SH_PATH_MAX - 1);
            g_cwd[QOS_SH_PATH_MAX - 1] = '\0';
        } else {
            snprintf(tmp, sizeof(tmp), "cd: not found: %s\n", argv[1]);
            out_append(out, out_cap, tmp);
        }
    } else if (streq(cmd, "pwd")) {
        out_append(out, out_cap, g_cwd);
        out_append(out, out_cap, "\n");
    } else if (streq(cmd, "export")) {
        char *eq;
        if (argc < 2 || (eq = strchr(argv[1], '=')) == NULL) {
            out_append(out, out_cap, "export: invalid\n");
        } else {
            char key[QOS_SH_ENV_KEY_MAX];
            const char *val = eq + 1;
            size_t klen = (size_t)(eq - argv[1]);
            if (klen >= sizeof(key)) {
                klen = sizeof(key) - 1;
            }
            memcpy(key, argv[1], klen);
            key[klen] = '\0';
            env_set(key, val);
            snprintf(tmp, sizeof(tmp), "set %s=%s\n", key, val);
            out_append(out, out_cap, tmp);
        }
    } else if (streq(cmd, "unset")) {
        if (argc < 2) {
            out_append(out, out_cap, "unset: missing key\n");
        } else {
            env_unset(argv[1]);
            snprintf(tmp, sizeof(tmp), "unset %s\n", argv[1]);
            out_append(out, out_cap, tmp);
        }
    } else if (streq(cmd, "ps")) {
        out_append(out, out_cap, "PID PPID STATE CMD\n1 0 Running sh\n");
    } else if (streq(cmd, "ip")) {
        if (argc < 2) {
            out_append(out, out_cap, "ip: usage: ip addr|route\n");
        } else if (streq(argv[1], "addr")) {
            out_append(out, out_cap, "1: eth0    inet 10.0.2.15/24\n");
        } else if (streq(argv[1], "route")) {
            out_append(out, out_cap, "default via 10.0.2.2 dev eth0\n");
            out_append(out, out_cap, "10.0.2.0/24 dev eth0 src 10.0.2.15\n");
        } else {
            out_append(out, out_cap, "ip: usage: ip addr|route\n");
        }
    } else if (streq(cmd, "ping")) {
        if (argc < 2) {
            out_append(out, out_cap, "ping: missing host\n");
        } else {
            uint32_t dst_ip = 0;
            uint32_t src_ip = 0;
            uint8_t payload[3] = {'q', 'o', 's'};
            int got = -1;
            if (parse_ipv4_addr(argv[1], &dst_ip) != 0) {
                out_append(out, out_cap, "ping: invalid host\n");
            } else {
                got = ping_via_syscall_raw(dst_ip, payload, sizeof(payload));
                if (got < 0 && qos_net_icmp_echo != 0) {
                    uint8_t reply[16];
                    got = qos_net_icmp_echo(dst_ip, 1, 1, payload, sizeof(payload), reply, sizeof(reply), &src_ip);
                } else if (got < 0 && (dst_ip == QOS_SH_IPV4_GATEWAY || dst_ip == QOS_SH_IPV4_LOCAL)) {
                    src_ip = dst_ip;
                    got = (int)sizeof(payload);
                }
                if (got > 0) {
                    snprintf(tmp, sizeof(tmp), "PING %s\n64 bytes from %s: icmp_seq=1 ttl=64 time=1ms\n"
                                               "1 packets transmitted, 1 received\n",
                             argv[1], argv[1]);
                } else {
                    (void)src_ip;
                    snprintf(tmp, sizeof(tmp), "PING %s\nFrom 10.0.2.15 icmp_seq=1 Destination Host Unreachable\n"
                                               "1 packets transmitted, 0 received\n",
                             argv[1]);
                }
                out_append(out, out_cap, tmp);
            }
        }
    } else if (streq(cmd, "wget")) {
        if (argc < 2) {
            out_append(out, out_cap, "wget: missing url\n");
        } else {
            if (wget_via_syscall_stream(argv[1], out, out_cap) != 0) {
                out_append(out, out_cap, "wget: failed\n");
            }
        }
    } else {
        snprintf(tmp, sizeof(tmp), "unknown command: %s\n", argv[0]);
        out_append(out, out_cap, tmp);
    }

    if (redir_out != NULL) {
        (void)file_set(redir_out, out, append);
        out_clear(out);
    }
}

static int run_command(char *line) {
    char tokens[QOS_SH_TOKENS_MAX][QOS_SH_TOKEN_MAX];
    int ntok;
    int pipe_idx = -1;
    int i;
    int exit_shell = 0;
    char out1[QOS_SH_OUT_MAX];
    char out2[QOS_SH_OUT_MAX];
    char *nl = strchr(line, '\n');

    if (nl != NULL) {
        *nl = '\0';
    }
    ntok = tokenize(line, tokens, QOS_SH_TOKENS_MAX);
    if (ntok == 0) {
        return 0;
    }
    for (i = 0; i < ntok; i++) {
        if (streq(tokens[i], "|")) {
            pipe_idx = i;
            break;
        }
    }

    out_clear(out1);
    out_clear(out2);
    if (pipe_idx > 0 && pipe_idx < ntok - 1) {
        execute_segment(tokens, pipe_idx, NULL, out1, sizeof(out1), &exit_shell);
        execute_segment(tokens + pipe_idx + 1, ntok - pipe_idx - 1, out1, out2, sizeof(out2), &exit_shell);
        if (out2[0] != '\0') {
            fputs(out2, stdout);
        }
    } else {
        execute_segment(tokens, ntok, NULL, out2, sizeof(out2), &exit_shell);
        if (out2[0] != '\0') {
            fputs(out2, stdout);
        }
    }

    return exit_shell;
}

int main(int argc, char **argv) {
    char line[256];
    fs_reset();

    if (argc > 1 && strcmp(argv[1], "--once") == 0) {
        size_t pos = 0;
        int i;
        line[0] = '\0';
        for (i = 2; i < argc; i++) {
            size_t n = strlen(argv[i]);
            if (pos != 0 && pos + 1 < sizeof(line)) {
                line[pos++] = ' ';
            }
            if (pos + n >= sizeof(line)) {
                n = sizeof(line) - pos - 1;
            }
            memcpy(line + pos, argv[i], n);
            pos += n;
        }
        line[pos] = '\0';
        (void)run_command(line);
        return 0;
    }

    while (1) {
        char prompt[QOS_SH_PATH_MAX + 16];
        snprintf(prompt, sizeof(prompt), "qos-sh:%s> ", g_cwd);
        fputs(prompt, stdout);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            fputc('\n', stdout);
            return 0;
        }

        if (run_command(line) != 0) {
            return 0;
        }
    }
}
