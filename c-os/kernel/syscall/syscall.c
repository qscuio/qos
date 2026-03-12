#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "../init_state.h"
#include "../kernel.h"
#include "../net/net.h"
#include "syscall.h"

#if defined(__GNUC__)
#define QOS_WEAK __attribute__((weak))
#else
#define QOS_WEAK
#endif

extern uint32_t qos_kernel_init_state(void) QOS_WEAK;
extern uint32_t qos_pmm_total_pages(void) QOS_WEAK;
extern uint32_t qos_pmm_free_pages(void) QOS_WEAK;
extern uint32_t qos_sched_count(void) QOS_WEAK;
extern uint32_t qos_vfs_count(void) QOS_WEAK;
extern uint32_t qos_net_queue_len(void) QOS_WEAK;
extern uint32_t qos_drivers_count(void) QOS_WEAK;
extern uint32_t qos_proc_count(void) QOS_WEAK;
extern int qos_proc_fork(uint32_t parent_pid, uint32_t child_pid) QOS_WEAK;
extern int qos_proc_exec_signal_reset(uint32_t pid) QOS_WEAK;
extern int qos_proc_exit(uint32_t pid, int32_t code) QOS_WEAK;
extern int32_t qos_proc_wait(uint32_t parent_pid, int32_t pid, int32_t *out_status, uint32_t options) QOS_WEAK;
extern int qos_proc_alive(uint32_t pid) QOS_WEAK;
extern int qos_proc_cwd_set(uint32_t pid, const char *path) QOS_WEAK;
extern int32_t qos_proc_cwd_get(uint32_t pid, char *out, uint32_t out_len) QOS_WEAK;
extern int qos_vfs_create(const char *path) QOS_WEAK;
extern int qos_vfs_exists(const char *path) QOS_WEAK;
extern int qos_vfs_remove(const char *path) QOS_WEAK;
extern int qos_vfs_fs_kind(const char *path) QOS_WEAK;
extern int qos_proc_signal_send(uint32_t pid, uint32_t signum) QOS_WEAK;
extern int qos_proc_signal_pending(uint32_t pid, uint32_t *out_pending) QOS_WEAK;
extern int qos_proc_signal_get_handler(uint32_t pid, uint32_t signum, uint64_t *out_handler) QOS_WEAK;
extern int qos_proc_signal_set_handler(uint32_t pid, uint32_t signum, uint64_t handler) QOS_WEAK;
extern int qos_proc_signal_mask(uint32_t pid, uint32_t *out_mask) QOS_WEAK;
extern int qos_proc_signal_set_mask(uint32_t pid, uint32_t mask) QOS_WEAK;
extern int qos_proc_signal_altstack_set(uint32_t pid, uint64_t sp, uint64_t size, uint32_t flags) QOS_WEAK;
extern int qos_proc_signal_altstack_get(uint32_t pid, qos_sigaltstack_t *out_altstack) QOS_WEAK;
extern int qos_proc_signal_sigreturn(uint32_t pid, const qos_sigframe_t *frame) QOS_WEAK;
extern int32_t qos_proc_signal_next(uint32_t pid) QOS_WEAK;
extern int qos_vmm_map(uint64_t va, uint64_t pa, uint32_t flags) QOS_WEAK;
extern int qos_vmm_unmap(uint64_t va) QOS_WEAK;
extern uint32_t qos_sched_next(void) QOS_WEAK;
extern int qos_net_enqueue_packet(const void *data, uint32_t len) QOS_WEAK;
extern int qos_net_dequeue_packet(void *out, uint32_t cap) QOS_WEAK;
extern int qos_net_udp_bind(uint16_t port) QOS_WEAK;
extern int qos_net_udp_unbind(uint16_t port) QOS_WEAK;
extern int qos_net_udp_send(uint16_t src_port, uint32_t src_ip, uint16_t dst_port, const void *data, uint32_t len) QOS_WEAK;
extern int qos_net_udp_recv(uint16_t dst_port, void *out, uint32_t cap, uint32_t *out_src_ip, uint16_t *out_src_port) QOS_WEAK;
extern int qos_net_icmp_echo(uint32_t dst_ip,
                             uint16_t ident,
                             uint16_t seq,
                             const void *payload,
                             uint32_t len,
                             void *out_reply,
                             uint32_t cap,
                             uint32_t *out_src_ip) QOS_WEAK;
extern int qos_net_tcp_listen(uint16_t port) QOS_WEAK;
extern int qos_net_tcp_unlisten(uint16_t port) QOS_WEAK;
extern int qos_net_tcp_connect(uint16_t local_port, uint32_t remote_ip, uint16_t remote_port) QOS_WEAK;
extern int qos_net_tcp_step(int conn_id, uint32_t event) QOS_WEAK;

static uint8_t g_used[QOS_SYSCALL_MAX];
static uint8_t g_ops[QOS_SYSCALL_MAX];
static int64_t g_values[QOS_SYSCALL_MAX];
static uint32_t g_count = 0;

#define QOS_FD_MAX 256U
#define QOS_FD_KIND_VFS 1U
#define QOS_FD_KIND_SOCKET 2U
#define QOS_FD_KIND_PIPE_R 3U
#define QOS_FD_KIND_PIPE_W 4U
#define QOS_FD_KIND_PROC 5U
#define QOS_SOCK_STREAM 1U
#define QOS_SOCK_DGRAM 2U
#define QOS_SOCK_RAW 3U
#define QOS_PROC_FD_NONE 0U
#define QOS_PROC_FD_MEMINFO 1U
#define QOS_PROC_FD_STATUS 2U
#define QOS_PIPE_MAX 32U
#define QOS_PIPE_CAP 1024U
#define QOS_FILE_MAX 128U
#define QOS_FILE_PATH_MAX 128U
#define QOS_VMM_PAGE_SIZE 4096ULL
#define QOS_VMM_DEFAULT_FLAGS 1U
#define QOS_SOCK_PORT_MIN 1024U
#define QOS_SOCK_PORT_MAX 65535U
#define QOS_SOCK_PORT_COUNT (QOS_SOCK_PORT_MAX - QOS_SOCK_PORT_MIN + 1U)
#define QOS_SOCK_PORT_WORD_BITS 32U
#define QOS_SOCK_PORT_WORDS ((QOS_SOCK_PORT_COUNT + QOS_SOCK_PORT_WORD_BITS - 1U) / QOS_SOCK_PORT_WORD_BITS)
#define QOS_SOCK_PENDING_MAX 16U

static uint8_t g_fd_used[QOS_FD_MAX];
static uint8_t g_fd_kind[QOS_FD_MAX];
static uint8_t g_fd_sock_bound[QOS_FD_MAX];
static uint8_t g_fd_sock_listening[QOS_FD_MAX];
static uint8_t g_fd_sock_connected[QOS_FD_MAX];
static uint8_t g_fd_sock_pending[QOS_FD_MAX];
static uint8_t g_fd_sock_backlog[QOS_FD_MAX];
static uint8_t g_fd_sock_type[QOS_FD_MAX];
static uint16_t g_fd_sock_lport[QOS_FD_MAX];
static uint16_t g_fd_sock_rport[QOS_FD_MAX];
static uint32_t g_fd_sock_rip[QOS_FD_MAX];
static int16_t g_fd_sock_conn_id[QOS_FD_MAX];
static uint16_t g_fd_sock_stream_len[QOS_FD_MAX];
static uint8_t g_fd_sock_stream_buf[QOS_FD_MAX][QOS_NET_MAX_PACKET];
static uint16_t g_fd_sock_raw_len[QOS_FD_MAX];
static uint32_t g_fd_sock_raw_src_ip[QOS_FD_MAX];
static uint8_t g_fd_sock_raw_buf[QOS_FD_MAX][QOS_NET_MAX_PACKET];
static uint16_t g_fd_sock_pending_rport[QOS_FD_MAX];
static uint32_t g_fd_sock_pending_rip[QOS_FD_MAX];
static int16_t g_fd_sock_pending_conn_id[QOS_FD_MAX];
static uint8_t g_fd_sock_pending_head[QOS_FD_MAX];
static uint8_t g_fd_sock_pending_tail[QOS_FD_MAX];
static uint16_t g_fd_sock_pending_q_rport[QOS_FD_MAX][QOS_SOCK_PENDING_MAX];
static uint32_t g_fd_sock_pending_q_rip[QOS_FD_MAX][QOS_SOCK_PENDING_MAX];
static int16_t g_fd_sock_pending_q_conn_id[QOS_FD_MAX][QOS_SOCK_PENDING_MAX];
static uint16_t g_fd_pipe_id[QOS_FD_MAX];
static uint64_t g_fd_offset[QOS_FD_MAX];
static uint16_t g_fd_file_id[QOS_FD_MAX];
static uint8_t g_fd_proc_kind[QOS_FD_MAX];
static uint32_t g_fd_proc_pid[QOS_FD_MAX];
static uint32_t g_sock_ports_stream[QOS_SOCK_PORT_WORDS];
static uint32_t g_sock_ports_dgram[QOS_SOCK_PORT_WORDS];

static uint8_t g_file_used[QOS_FILE_MAX];
static char g_file_paths[QOS_FILE_MAX][QOS_FILE_PATH_MAX];
static uint16_t g_file_len[QOS_FILE_MAX];
static uint8_t g_file_data[QOS_FILE_MAX][QOS_PIPE_CAP];

static uint8_t g_pipe_used[QOS_PIPE_MAX];
static uint16_t g_pipe_len[QOS_PIPE_MAX];
static uint8_t g_pipe_buf[QOS_PIPE_MAX][QOS_PIPE_CAP];

static int fd_valid(uint32_t fd) {
    return fd < QOS_FD_MAX && g_fd_used[fd] != 0;
}

static uint32_t *socket_port_bitmap(uint8_t sock_type) {
    if (sock_type == QOS_SOCK_STREAM) {
        return g_sock_ports_stream;
    }
    if (sock_type == QOS_SOCK_DGRAM) {
        return g_sock_ports_dgram;
    }
    return 0;
}

static int socket_port_index(uint16_t port, uint32_t *out_word, uint32_t *out_mask) {
    uint32_t idx;
    if (port < QOS_SOCK_PORT_MIN || port > QOS_SOCK_PORT_MAX || out_word == 0 || out_mask == 0) {
        return -1;
    }
    idx = (uint32_t)port - QOS_SOCK_PORT_MIN;
    *out_word = idx / QOS_SOCK_PORT_WORD_BITS;
    *out_mask = 1u << (idx % QOS_SOCK_PORT_WORD_BITS);
    return 0;
}

static int socket_port_mark(uint8_t sock_type, uint16_t port) {
    uint32_t *bitmap = socket_port_bitmap(sock_type);
    uint32_t word = 0;
    uint32_t mask = 0;
    if (bitmap == 0 || socket_port_index(port, &word, &mask) != 0) {
        return -1;
    }
    if ((bitmap[word] & mask) != 0) {
        return -1;
    }
    bitmap[word] |= mask;
    return 0;
}

static void socket_port_unmark(uint8_t sock_type, uint16_t port) {
    uint32_t *bitmap = socket_port_bitmap(sock_type);
    uint32_t word = 0;
    uint32_t mask = 0;
    if (bitmap == 0 || socket_port_index(port, &word, &mask) != 0) {
        return;
    }
    bitmap[word] &= ~mask;
}

static uint16_t socket_port_alloc(uint8_t sock_type) {
    uint32_t port;
    for (port = 49152U; port <= QOS_SOCK_PORT_MAX; port++) {
        if (socket_port_mark(sock_type, (uint16_t)port) == 0) {
            return (uint16_t)port;
        }
    }
    for (port = QOS_SOCK_PORT_MIN; port < 49152U; port++) {
        if (socket_port_mark(sock_type, (uint16_t)port) == 0) {
            return (uint16_t)port;
        }
    }
    return 0;
}

static int socket_port_in_use(uint16_t port, uint8_t sock_type) {
    uint32_t i;
    for (i = 0; i < QOS_FD_MAX; i++) {
        if (g_fd_used[i] != 0 && g_fd_kind[i] == QOS_FD_KIND_SOCKET && g_fd_sock_bound[i] != 0 &&
            g_fd_sock_type[i] == sock_type && g_fd_sock_lport[i] == port) {
            return 1;
        }
    }
    return 0;
}

static int socket_listener_in_use(uint16_t port) {
    uint32_t i;
    for (i = 0; i < QOS_FD_MAX; i++) {
        if (g_fd_used[i] != 0 && g_fd_kind[i] == QOS_FD_KIND_SOCKET && g_fd_sock_type[i] == QOS_SOCK_STREAM &&
            g_fd_sock_listening[i] != 0 && g_fd_sock_lport[i] == port) {
            return 1;
        }
    }
    return 0;
}

static int socket_pending_push(uint32_t fd, uint16_t peer_port, uint32_t peer_ip, int16_t conn_id) {
    uint8_t tail;
    if (fd >= QOS_FD_MAX || g_fd_sock_pending[fd] >= QOS_SOCK_PENDING_MAX) {
        return -1;
    }
    tail = g_fd_sock_pending_tail[fd];
    g_fd_sock_pending_q_rport[fd][tail] = peer_port;
    g_fd_sock_pending_q_rip[fd][tail] = peer_ip;
    g_fd_sock_pending_q_conn_id[fd][tail] = conn_id;
    g_fd_sock_pending_tail[fd] = (uint8_t)((tail + 1U) % QOS_SOCK_PENDING_MAX);
    g_fd_sock_pending[fd]++;
    return 0;
}

static int socket_pending_pop(uint32_t fd, uint16_t *out_port, uint32_t *out_ip, int16_t *out_conn_id) {
    uint8_t head;
    if (fd >= QOS_FD_MAX || g_fd_sock_pending[fd] == 0 || out_port == 0 || out_ip == 0 || out_conn_id == 0) {
        return -1;
    }
    head = g_fd_sock_pending_head[fd];
    *out_port = g_fd_sock_pending_q_rport[fd][head];
    *out_ip = g_fd_sock_pending_q_rip[fd][head];
    *out_conn_id = g_fd_sock_pending_q_conn_id[fd][head];
    g_fd_sock_pending_q_rport[fd][head] = 0;
    g_fd_sock_pending_q_rip[fd][head] = 0;
    g_fd_sock_pending_q_conn_id[fd][head] = -1;
    g_fd_sock_pending_head[fd] = (uint8_t)((head + 1U) % QOS_SOCK_PENDING_MAX);
    g_fd_sock_pending[fd]--;
    if (g_fd_sock_pending[fd] == 0) {
        g_fd_sock_pending_head[fd] = 0;
        g_fd_sock_pending_tail[fd] = 0;
    }
    return 0;
}

static int fd_alloc(uint8_t kind) {
    uint32_t i;
    for (i = 0; i < QOS_FD_MAX; i++) {
        if (g_fd_used[i] == 0) {
            g_fd_used[i] = 1;
            g_fd_kind[i] = kind;
            g_fd_sock_bound[i] = 0;
            g_fd_sock_listening[i] = 0;
            g_fd_sock_connected[i] = 0;
            g_fd_sock_pending[i] = 0;
            g_fd_sock_backlog[i] = 0;
            g_fd_sock_type[i] = 0;
            g_fd_sock_lport[i] = 0;
            g_fd_sock_rport[i] = 0;
            g_fd_sock_rip[i] = 0;
            g_fd_sock_conn_id[i] = -1;
            g_fd_sock_stream_len[i] = 0;
            memset(g_fd_sock_stream_buf[i], 0, sizeof(g_fd_sock_stream_buf[i]));
            g_fd_sock_raw_len[i] = 0;
            g_fd_sock_raw_src_ip[i] = 0;
            memset(g_fd_sock_raw_buf[i], 0, sizeof(g_fd_sock_raw_buf[i]));
            g_fd_sock_pending_rport[i] = 0;
            g_fd_sock_pending_rip[i] = 0;
            g_fd_sock_pending_conn_id[i] = -1;
            g_fd_sock_pending_head[i] = 0;
            g_fd_sock_pending_tail[i] = 0;
            memset(g_fd_sock_pending_q_rport[i], 0, sizeof(g_fd_sock_pending_q_rport[i]));
            memset(g_fd_sock_pending_q_rip[i], 0, sizeof(g_fd_sock_pending_q_rip[i]));
            memset(g_fd_sock_pending_q_conn_id[i], 0xFF, sizeof(g_fd_sock_pending_q_conn_id[i]));
            g_fd_pipe_id[i] = 0;
            g_fd_offset[i] = 0;
            g_fd_file_id[i] = UINT16_MAX;
            g_fd_proc_kind[i] = QOS_PROC_FD_NONE;
            g_fd_proc_pid[i] = 0;
            return (int)i;
        }
    }
    return -1;
}

static int file_find(const char *path) {
    uint32_t i;
    for (i = 0; i < QOS_FILE_MAX; i++) {
        if (g_file_used[i] != 0 && strcmp(g_file_paths[i], path) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static int file_alloc(const char *path) {
    uint32_t i;
    for (i = 0; i < QOS_FILE_MAX; i++) {
        if (g_file_used[i] == 0) {
            g_file_used[i] = 1;
            strncpy(g_file_paths[i], path, QOS_FILE_PATH_MAX - 1);
            g_file_paths[i][QOS_FILE_PATH_MAX - 1] = '\0';
            g_file_len[i] = 0;
            memset(g_file_data[i], 0, QOS_PIPE_CAP);
            return (int)i;
        }
    }
    return -1;
}

static int file_ensure(const char *path) {
    int idx = file_find(path);
    if (idx >= 0) {
        return idx;
    }
    return file_alloc(path);
}

static int file_has_refs(uint16_t file_id) {
    uint32_t i;
    for (i = 0; i < QOS_FD_MAX; i++) {
        if (g_fd_used[i] != 0 && g_fd_kind[i] == QOS_FD_KIND_VFS && g_fd_file_id[i] == file_id) {
            return 1;
        }
    }
    return 0;
}

static void file_drop(uint16_t file_id) {
    if (file_id >= QOS_FILE_MAX) {
        return;
    }
    g_file_used[file_id] = 0;
    g_file_paths[file_id][0] = '\0';
    g_file_len[file_id] = 0;
    memset(g_file_data[file_id], 0, QOS_PIPE_CAP);
}

static void file_unlink_path(const char *path) {
    int idx = file_find(path);
    if (idx < 0) {
        return;
    }
    g_file_paths[(uint32_t)idx][0] = '\0';
    if (file_has_refs((uint16_t)idx) == 0) {
        file_drop((uint16_t)idx);
    }
}

static int proc_path_kind(const char *path, uint8_t *out_kind, uint32_t *out_pid) {
    size_t len;
    size_t i;
    uint32_t pid = 0;
    const char suffix[] = "/status";
    if (path == 0 || out_kind == 0 || out_pid == 0) {
        return -1;
    }
    if (strcmp(path, "/proc/meminfo") == 0) {
        *out_kind = QOS_PROC_FD_MEMINFO;
        *out_pid = 0;
        return 0;
    }
    if (strncmp(path, "/proc/", 6) != 0) {
        return -1;
    }
    len = strlen(path);
    if (len <= 6 + sizeof(suffix) - 1) {
        return -1;
    }
    if (strcmp(path + (len - (sizeof(suffix) - 1)), suffix) != 0) {
        return -1;
    }
    for (i = 6; i < len - (sizeof(suffix) - 1); i++) {
        uint8_t ch = (uint8_t)path[i];
        if (ch < '0' || ch > '9') {
            return -1;
        }
        pid = (pid * 10u) + (uint32_t)(ch - '0');
    }
    if (pid == 0) {
        return -1;
    }
    *out_kind = QOS_PROC_FD_STATUS;
    *out_pid = pid;
    return 0;
}

static uint32_t proc_fd_render(uint32_t fd, char *out, uint32_t cap) {
    char tmp[256];
    uint8_t kind;
    uint32_t pid;
    int n;
    uint32_t total;
    uint32_t copy_len;
    uint32_t total_pages = qos_pmm_total_pages != 0 ? qos_pmm_total_pages() : 0;
    uint32_t free_pages = qos_pmm_free_pages != 0 ? qos_pmm_free_pages() : 0;
    uint32_t proc_total = qos_proc_count != 0 ? qos_proc_count() : 0;

    if (fd >= QOS_FD_MAX || g_fd_kind[fd] != QOS_FD_KIND_PROC) {
        return 0;
    }

    kind = g_fd_proc_kind[fd];
    pid = g_fd_proc_pid[fd];
    if (kind == QOS_PROC_FD_MEMINFO) {
        n = snprintf(tmp, sizeof(tmp), "MemTotal:\t%u kB\nMemFree:\t%u kB\nProcCount:\t%u\n", total_pages * 4u,
                     free_pages * 4u, proc_total);
    } else if (kind == QOS_PROC_FD_STATUS) {
        const char *state = (qos_proc_alive != 0 && qos_proc_alive(pid) != 0) ? "Running" : "Zombie";
        n = snprintf(tmp, sizeof(tmp), "Pid:\t%u\nState:\t%s\n", pid, state);
    } else {
        return 0;
    }

    if (n < 0) {
        return 0;
    }
    total = (uint32_t)n;
    if (out == 0 || cap == 0) {
        return total;
    }
    copy_len = total;
    if (copy_len > cap) {
        copy_len = cap;
    }
    memcpy(out, tmp, copy_len);
    return total;
}

static int parse_sockaddr(const void *addr, uint32_t len, uint16_t *out_port, uint32_t *out_ip, int allow_any_ip) {
    const uint8_t *p = (const uint8_t *)addr;
    if (addr == 0 || len < 8 || out_port == 0 || out_ip == 0) {
        return -1;
    }
    *out_port = ((uint16_t)p[2] << 8) | (uint16_t)p[3];
    *out_ip = ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16) | ((uint32_t)p[6] << 8) | (uint32_t)p[7];
    if (*out_port == 0) {
        return -1;
    }
    if (allow_any_ip == 0 && *out_ip == 0) {
        return -1;
    }
    return 0;
}

static void write_sockaddr(void *addr, uint16_t port, uint32_t ip) {
    uint8_t *p;
    if (addr == 0) {
        return;
    }
    p = (uint8_t *)addr;
    memset(p, 0, 16);
    p[0] = 2;
    p[1] = 0;
    p[2] = (uint8_t)(port >> 8);
    p[3] = (uint8_t)(port & 0xFF);
    p[4] = (uint8_t)(ip >> 24);
    p[5] = (uint8_t)((ip >> 16) & 0xFF);
    p[6] = (uint8_t)((ip >> 8) & 0xFF);
    p[7] = (uint8_t)(ip & 0xFF);
}

static int stream_host_response_prepare(uint32_t fd, const uint8_t *req, uint32_t req_len) {
    static const char k_prefix[] = "GET ";
    static const char k_resp[] = "HTTP/1.0 200 OK\r\nContent-Length: 4\r\n\r\nqos\n";
    uint32_t resp_len = (uint32_t)(sizeof(k_resp) - 1);
    if (fd >= QOS_FD_MAX || req == 0 || req_len < sizeof(k_prefix) - 1 || memcmp(req, k_prefix, sizeof(k_prefix) - 1) != 0 ||
        resp_len > QOS_NET_MAX_PACKET) {
        return -1;
    }
    memcpy(g_fd_sock_stream_buf[fd], k_resp, resp_len);
    g_fd_sock_stream_len[fd] = (uint16_t)resp_len;
    return 0;
}

static int pipe_in_use(uint16_t pipe_id) {
    uint32_t i;
    for (i = 0; i < QOS_FD_MAX; i++) {
        if (g_fd_used[i] != 0 && (g_fd_kind[i] == QOS_FD_KIND_PIPE_R || g_fd_kind[i] == QOS_FD_KIND_PIPE_W) &&
            g_fd_pipe_id[i] == pipe_id) {
            return 1;
        }
    }
    return 0;
}

static int pipe_alloc(void) {
    uint32_t i;
    for (i = 0; i < QOS_PIPE_MAX; i++) {
        if (g_pipe_used[i] == 0) {
            g_pipe_used[i] = 1;
            g_pipe_len[i] = 0;
            memset(g_pipe_buf[i], 0, QOS_PIPE_CAP);
            return (int)i;
        }
    }
    return -1;
}

static void fd_clear(uint32_t fd) {
    uint16_t pipe_id;
    uint16_t file_id;
    uint8_t was_socket;
    uint8_t sock_type;
    uint8_t sock_listening;
    uint8_t sock_bound;
    uint16_t sock_lport;
    if (!fd_valid(fd)) {
        return;
    }

    pipe_id = g_fd_pipe_id[fd];
    file_id = g_fd_file_id[fd];
    was_socket = g_fd_kind[fd] == QOS_FD_KIND_SOCKET;
    sock_type = g_fd_sock_type[fd];
    sock_listening = g_fd_sock_listening[fd];
    sock_bound = g_fd_sock_bound[fd];
    sock_lport = g_fd_sock_lport[fd];

    g_fd_used[fd] = 0;
    g_fd_kind[fd] = 0;
    g_fd_sock_bound[fd] = 0;
    g_fd_sock_listening[fd] = 0;
    g_fd_sock_connected[fd] = 0;
    g_fd_sock_pending[fd] = 0;
    g_fd_sock_backlog[fd] = 0;
    g_fd_sock_type[fd] = 0;
    g_fd_sock_lport[fd] = 0;
    g_fd_sock_rport[fd] = 0;
    g_fd_sock_rip[fd] = 0;
    g_fd_sock_conn_id[fd] = -1;
    g_fd_sock_stream_len[fd] = 0;
    memset(g_fd_sock_stream_buf[fd], 0, sizeof(g_fd_sock_stream_buf[fd]));
    g_fd_sock_raw_len[fd] = 0;
    g_fd_sock_raw_src_ip[fd] = 0;
    memset(g_fd_sock_raw_buf[fd], 0, sizeof(g_fd_sock_raw_buf[fd]));
    g_fd_sock_pending_rport[fd] = 0;
    g_fd_sock_pending_rip[fd] = 0;
    g_fd_sock_pending_conn_id[fd] = -1;
    g_fd_sock_pending_head[fd] = 0;
    g_fd_sock_pending_tail[fd] = 0;
    memset(g_fd_sock_pending_q_rport[fd], 0, sizeof(g_fd_sock_pending_q_rport[fd]));
    memset(g_fd_sock_pending_q_rip[fd], 0, sizeof(g_fd_sock_pending_q_rip[fd]));
    memset(g_fd_sock_pending_q_conn_id[fd], 0xFF, sizeof(g_fd_sock_pending_q_conn_id[fd]));
    g_fd_pipe_id[fd] = 0;
    g_fd_offset[fd] = 0;
    g_fd_file_id[fd] = UINT16_MAX;
    g_fd_proc_kind[fd] = QOS_PROC_FD_NONE;
    g_fd_proc_pid[fd] = 0;

    if (pipe_id < QOS_PIPE_MAX && pipe_in_use(pipe_id) == 0) {
        g_pipe_used[pipe_id] = 0;
        g_pipe_len[pipe_id] = 0;
        memset(g_pipe_buf[pipe_id], 0, QOS_PIPE_CAP);
    }

    if (file_id < QOS_FILE_MAX && g_file_used[file_id] != 0 && g_file_paths[file_id][0] == '\0' &&
        file_has_refs(file_id) == 0) {
        file_drop(file_id);
    }

    if (was_socket != 0 && sock_type == QOS_SOCK_STREAM && sock_listening != 0 &&
        socket_listener_in_use(sock_lport) == 0 && qos_net_tcp_unlisten != 0) {
        (void)qos_net_tcp_unlisten(sock_lport);
    }

    if (was_socket != 0 && sock_bound != 0 && socket_port_in_use(sock_lport, sock_type) == 0) {
        if (sock_type == QOS_SOCK_DGRAM && qos_net_udp_unbind != 0) {
            (void)qos_net_udp_unbind(sock_lport);
        }
        socket_port_unmark(sock_type, sock_lport);
    }
}

static int64_t syscall_query_value(uint32_t query) {
    switch (query) {
        case SYSCALL_QUERY_INIT_STATE:
            return qos_kernel_init_state != 0 ? (int64_t)qos_kernel_init_state() : -1;
        case SYSCALL_QUERY_PMM_TOTAL:
            return qos_pmm_total_pages != 0 ? (int64_t)qos_pmm_total_pages() : -1;
        case SYSCALL_QUERY_PMM_FREE:
            return qos_pmm_free_pages != 0 ? (int64_t)qos_pmm_free_pages() : -1;
        case SYSCALL_QUERY_SCHED_COUNT:
            return qos_sched_count != 0 ? (int64_t)qos_sched_count() : -1;
        case SYSCALL_QUERY_VFS_COUNT:
            return qos_vfs_count != 0 ? (int64_t)qos_vfs_count() : -1;
        case SYSCALL_QUERY_NET_QUEUE_LEN:
            return qos_net_queue_len != 0 ? (int64_t)qos_net_queue_len() : -1;
        case SYSCALL_QUERY_DRIVERS_COUNT:
            return qos_drivers_count != 0 ? (int64_t)qos_drivers_count() : -1;
        case SYSCALL_QUERY_SYSCALL_COUNT:
            return (int64_t)qos_syscall_count();
        case SYSCALL_QUERY_PROC_COUNT:
            return qos_proc_count != 0 ? (int64_t)qos_proc_count() : -1;
        default:
            return -1;
    }
}

static void syscall_register_default_queries(void) {
    (void)qos_syscall_register(SYSCALL_NR_QUERY_INIT_STATE, SYSCALL_OP_QUERY, SYSCALL_QUERY_INIT_STATE);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_PMM_TOTAL, SYSCALL_OP_QUERY, SYSCALL_QUERY_PMM_TOTAL);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_PMM_FREE, SYSCALL_OP_QUERY, SYSCALL_QUERY_PMM_FREE);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_SCHED_COUNT, SYSCALL_OP_QUERY, SYSCALL_QUERY_SCHED_COUNT);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_VFS_COUNT, SYSCALL_OP_QUERY, SYSCALL_QUERY_VFS_COUNT);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_NET_QUEUE_LEN, SYSCALL_OP_QUERY, SYSCALL_QUERY_NET_QUEUE_LEN);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_DRIVERS_COUNT, SYSCALL_OP_QUERY, SYSCALL_QUERY_DRIVERS_COUNT);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_SYSCALL_COUNT, SYSCALL_OP_QUERY, SYSCALL_QUERY_SYSCALL_COUNT);
    (void)qos_syscall_register(SYSCALL_NR_QUERY_PROC_COUNT, SYSCALL_OP_QUERY, SYSCALL_QUERY_PROC_COUNT);
}

static void syscall_register_default_signal_ops(void) {
    (void)qos_syscall_register(SYSCALL_NR_FORK, SYSCALL_OP_FORK, 0);
    (void)qos_syscall_register(SYSCALL_NR_EXEC, SYSCALL_OP_EXEC, 0);
    (void)qos_syscall_register(SYSCALL_NR_EXIT, SYSCALL_OP_EXIT, 0);
    (void)qos_syscall_register(SYSCALL_NR_GETPID, SYSCALL_OP_GETPID, 0);
    (void)qos_syscall_register(SYSCALL_NR_WAITPID, SYSCALL_OP_WAITPID, 0);
    (void)qos_syscall_register(SYSCALL_NR_OPEN, SYSCALL_OP_OPEN, 0);
    (void)qos_syscall_register(SYSCALL_NR_READ, SYSCALL_OP_READ, 0);
    (void)qos_syscall_register(SYSCALL_NR_WRITE, SYSCALL_OP_WRITE, 0);
    (void)qos_syscall_register(SYSCALL_NR_CLOSE, SYSCALL_OP_CLOSE, 0);
    (void)qos_syscall_register(SYSCALL_NR_DUP2, SYSCALL_OP_DUP2, 0);
    (void)qos_syscall_register(SYSCALL_NR_MMAP, SYSCALL_OP_MMAP, 0);
    (void)qos_syscall_register(SYSCALL_NR_MUNMAP, SYSCALL_OP_MUNMAP, 0);
    (void)qos_syscall_register(SYSCALL_NR_YIELD, SYSCALL_OP_YIELD, 0);
    (void)qos_syscall_register(SYSCALL_NR_SLEEP, SYSCALL_OP_SLEEP, 0);
    (void)qos_syscall_register(SYSCALL_NR_SOCKET, SYSCALL_OP_SOCKET, 0);
    (void)qos_syscall_register(SYSCALL_NR_BIND, SYSCALL_OP_BIND, 0);
    (void)qos_syscall_register(SYSCALL_NR_LISTEN, SYSCALL_OP_LISTEN, 0);
    (void)qos_syscall_register(SYSCALL_NR_ACCEPT, SYSCALL_OP_ACCEPT, 0);
    (void)qos_syscall_register(SYSCALL_NR_CONNECT, SYSCALL_OP_CONNECT, 0);
    (void)qos_syscall_register(SYSCALL_NR_SEND, SYSCALL_OP_SEND, 0);
    (void)qos_syscall_register(SYSCALL_NR_RECV, SYSCALL_OP_RECV, 0);
    (void)qos_syscall_register(SYSCALL_NR_STAT, SYSCALL_OP_STAT, 0);
    (void)qos_syscall_register(SYSCALL_NR_MKDIR, SYSCALL_OP_MKDIR, 0);
    (void)qos_syscall_register(SYSCALL_NR_UNLINK, SYSCALL_OP_UNLINK, 0);
    (void)qos_syscall_register(SYSCALL_NR_GETDENTS, SYSCALL_OP_GETDENTS, 0);
    (void)qos_syscall_register(SYSCALL_NR_LSEEK, SYSCALL_OP_LSEEK, 0);
    (void)qos_syscall_register(SYSCALL_NR_PIPE, SYSCALL_OP_PIPE, 0);
    (void)qos_syscall_register(SYSCALL_NR_CHDIR, SYSCALL_OP_CHDIR, 0);
    (void)qos_syscall_register(SYSCALL_NR_GETCWD, SYSCALL_OP_GETCWD, 0);
    (void)qos_syscall_register(SYSCALL_NR_KILL, SYSCALL_OP_KILL, 0);
    (void)qos_syscall_register(SYSCALL_NR_SIGACTION, SYSCALL_OP_SIGACTION, 0);
    (void)qos_syscall_register(SYSCALL_NR_SIGPROCMASK, SYSCALL_OP_SIGPROCMASK, 0);
    (void)qos_syscall_register(SYSCALL_NR_SIGRETURN, SYSCALL_OP_SIGRETURN, 0);
    (void)qos_syscall_register(SYSCALL_NR_SIGPENDING, SYSCALL_OP_SIGPENDING, 0);
    (void)qos_syscall_register(SYSCALL_NR_SIGSUSPEND, SYSCALL_OP_SIGSUSPEND, 0);
    (void)qos_syscall_register(SYSCALL_NR_SIGALTSTACK, SYSCALL_OP_SIGALTSTACK, 0);
}

void qos_syscall_reset(void) {
    memset(g_used, 0, sizeof(g_used));
    memset(g_ops, 0, sizeof(g_ops));
    memset(g_values, 0, sizeof(g_values));
    memset(g_fd_used, 0, sizeof(g_fd_used));
    memset(g_fd_kind, 0, sizeof(g_fd_kind));
    memset(g_fd_sock_bound, 0, sizeof(g_fd_sock_bound));
    memset(g_fd_sock_listening, 0, sizeof(g_fd_sock_listening));
    memset(g_fd_sock_connected, 0, sizeof(g_fd_sock_connected));
    memset(g_fd_sock_pending, 0, sizeof(g_fd_sock_pending));
    memset(g_fd_sock_backlog, 0, sizeof(g_fd_sock_backlog));
    memset(g_fd_sock_type, 0, sizeof(g_fd_sock_type));
    memset(g_fd_sock_lport, 0, sizeof(g_fd_sock_lport));
    memset(g_fd_sock_rport, 0, sizeof(g_fd_sock_rport));
    memset(g_fd_sock_rip, 0, sizeof(g_fd_sock_rip));
    memset(g_fd_sock_stream_len, 0, sizeof(g_fd_sock_stream_len));
    memset(g_fd_sock_stream_buf, 0, sizeof(g_fd_sock_stream_buf));
    memset(g_fd_sock_raw_len, 0, sizeof(g_fd_sock_raw_len));
    memset(g_fd_sock_raw_src_ip, 0, sizeof(g_fd_sock_raw_src_ip));
    memset(g_fd_sock_raw_buf, 0, sizeof(g_fd_sock_raw_buf));
    memset(g_fd_sock_pending_rport, 0, sizeof(g_fd_sock_pending_rport));
    memset(g_fd_sock_pending_rip, 0, sizeof(g_fd_sock_pending_rip));
    memset(g_fd_sock_pending_head, 0, sizeof(g_fd_sock_pending_head));
    memset(g_fd_sock_pending_tail, 0, sizeof(g_fd_sock_pending_tail));
    memset(g_fd_sock_pending_q_rport, 0, sizeof(g_fd_sock_pending_q_rport));
    memset(g_fd_sock_pending_q_rip, 0, sizeof(g_fd_sock_pending_q_rip));
    memset(g_fd_sock_pending_q_conn_id, 0xFF, sizeof(g_fd_sock_pending_q_conn_id));
    memset(g_fd_pipe_id, 0, sizeof(g_fd_pipe_id));
    memset(g_fd_offset, 0, sizeof(g_fd_offset));
    memset(g_fd_proc_kind, 0, sizeof(g_fd_proc_kind));
    memset(g_fd_proc_pid, 0, sizeof(g_fd_proc_pid));
    memset(g_sock_ports_stream, 0, sizeof(g_sock_ports_stream));
    memset(g_sock_ports_dgram, 0, sizeof(g_sock_ports_dgram));
    {
        uint32_t i;
        for (i = 0; i < QOS_FD_MAX; i++) {
            g_fd_file_id[i] = UINT16_MAX;
            g_fd_sock_conn_id[i] = -1;
            g_fd_sock_pending_conn_id[i] = -1;
        }
    }
    memset(g_file_used, 0, sizeof(g_file_used));
    memset(g_file_paths, 0, sizeof(g_file_paths));
    memset(g_file_len, 0, sizeof(g_file_len));
    memset(g_file_data, 0, sizeof(g_file_data));
    memset(g_pipe_used, 0, sizeof(g_pipe_used));
    memset(g_pipe_len, 0, sizeof(g_pipe_len));
    memset(g_pipe_buf, 0, sizeof(g_pipe_buf));
    g_count = 0;
}

int qos_syscall_register(uint32_t nr, uint32_t op, int64_t value) {
    if (nr >= QOS_SYSCALL_MAX) {
        return -1;
    }

    if (op != SYSCALL_OP_CONST && op != SYSCALL_OP_ADD && op != SYSCALL_OP_QUERY && op != SYSCALL_OP_KILL &&
        op != SYSCALL_OP_SIGPENDING && op != SYSCALL_OP_SIGACTION && op != SYSCALL_OP_SIGPROCMASK &&
        op != SYSCALL_OP_SIGALTSTACK && op != SYSCALL_OP_SIGSUSPEND && op != SYSCALL_OP_FORK &&
        op != SYSCALL_OP_EXEC && op != SYSCALL_OP_SIGRETURN && op != SYSCALL_OP_EXIT &&
        op != SYSCALL_OP_GETPID && op != SYSCALL_OP_WAITPID && op != SYSCALL_OP_STAT &&
        op != SYSCALL_OP_MKDIR && op != SYSCALL_OP_UNLINK && op != SYSCALL_OP_CHDIR &&
        op != SYSCALL_OP_GETCWD && op != SYSCALL_OP_OPEN && op != SYSCALL_OP_READ &&
        op != SYSCALL_OP_WRITE && op != SYSCALL_OP_CLOSE && op != SYSCALL_OP_DUP2 &&
        op != SYSCALL_OP_MMAP && op != SYSCALL_OP_MUNMAP && op != SYSCALL_OP_YIELD &&
        op != SYSCALL_OP_SLEEP && op != SYSCALL_OP_SOCKET && op != SYSCALL_OP_BIND &&
        op != SYSCALL_OP_LISTEN && op != SYSCALL_OP_ACCEPT && op != SYSCALL_OP_CONNECT &&
        op != SYSCALL_OP_SEND && op != SYSCALL_OP_RECV && op != SYSCALL_OP_GETDENTS &&
        op != SYSCALL_OP_LSEEK && op != SYSCALL_OP_PIPE) {
        return -1;
    }

    if (g_used[nr] != 0) {
        return -1;
    }

    g_used[nr] = 1;
    g_ops[nr] = (uint8_t)op;
    g_values[nr] = value;
    g_count++;
    return 0;
}

int qos_syscall_unregister(uint32_t nr) {
    if (nr >= QOS_SYSCALL_MAX || g_used[nr] == 0) {
        return -1;
    }

    g_used[nr] = 0;
    g_ops[nr] = 0;
    g_values[nr] = 0;
    g_count--;
    return 0;
}

int64_t qos_syscall_dispatch(uint32_t nr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    if (nr >= QOS_SYSCALL_MAX || g_used[nr] == 0) {
        return -1;
    }

    if (g_ops[nr] == SYSCALL_OP_CONST) {
        return g_values[nr];
    }

    if (g_ops[nr] == SYSCALL_OP_ADD) {
        return (int64_t)(a0 + a1);
    }

    if (g_ops[nr] == SYSCALL_OP_QUERY) {
        return syscall_query_value((uint32_t)g_values[nr]);
    }

    if (g_ops[nr] == SYSCALL_OP_OPEN) {
        const char *path = (const char *)(uintptr_t)a0;
        int fd;
        int file_id;
        uint8_t proc_kind = QOS_PROC_FD_NONE;
        uint32_t proc_pid = 0;
        if (qos_vfs_exists == 0 || path == 0) {
            return -1;
        }
        if (qos_vfs_exists(path) == 0) {
            if (proc_path_kind(path, &proc_kind, &proc_pid) != 0) {
                return -1;
            }
            if (proc_kind == QOS_PROC_FD_STATUS && (qos_proc_alive == 0 || qos_proc_alive(proc_pid) == 0)) {
                return -1;
            }
            fd = fd_alloc(QOS_FD_KIND_PROC);
            if (fd < 0) {
                return -1;
            }
            g_fd_proc_kind[(uint32_t)fd] = proc_kind;
            g_fd_proc_pid[(uint32_t)fd] = proc_pid;
            return (int64_t)fd;
        }
        fd = fd_alloc(QOS_FD_KIND_VFS);
        if (fd < 0) {
            return -1;
        }
        file_id = file_ensure(path);
        if (file_id < 0) {
            fd_clear((uint32_t)fd);
            return -1;
        }
        g_fd_file_id[(uint32_t)fd] = (uint16_t)file_id;
        return (int64_t)fd;
    }

    if (g_ops[nr] == SYSCALL_OP_READ) {
        uint32_t fd = (uint32_t)a0;
        uint8_t *out = (uint8_t *)(uintptr_t)a1;
        uint32_t len = (uint32_t)a2;
        if (!fd_valid(fd) || out == 0) {
            return -1;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_PROC) {
            uint32_t offset = (uint32_t)g_fd_offset[fd];
            char tmp[256];
            uint32_t total = proc_fd_render(fd, tmp, sizeof(tmp));
            uint32_t copy_len;
            if (offset >= total) {
                return 0;
            }
            copy_len = total - offset;
            if (copy_len > len) {
                copy_len = len;
            }
            if (copy_len == 0) {
                return 0;
            }
            memcpy(out, tmp + offset, copy_len);
            g_fd_offset[fd] = (uint64_t)(offset + copy_len);
            return (int64_t)copy_len;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_VFS) {
            uint16_t file_id = g_fd_file_id[fd];
            uint32_t offset = (uint32_t)g_fd_offset[fd];
            uint32_t avail;
            uint32_t copy_len;
            if (file_id >= QOS_FILE_MAX || g_file_used[file_id] == 0) {
                return -1;
            }
            if (offset >= g_file_len[file_id]) {
                return 0;
            }
            avail = (uint32_t)g_file_len[file_id] - offset;
            copy_len = len < avail ? len : avail;
            if (copy_len == 0) {
                return 0;
            }
            memcpy(out, g_file_data[file_id] + offset, copy_len);
            g_fd_offset[fd] = (uint64_t)(offset + copy_len);
            return (int64_t)copy_len;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_PIPE_R) {
            uint16_t pipe_id = g_fd_pipe_id[fd];
            uint32_t copy_len;
            if (pipe_id >= QOS_PIPE_MAX || g_pipe_used[pipe_id] == 0 || g_pipe_len[pipe_id] == 0) {
                return -1;
            }
            copy_len = g_pipe_len[pipe_id];
            if (copy_len > len) {
                copy_len = len;
            }
            if (copy_len == 0) {
                return 0;
            }
            memcpy(out, g_pipe_buf[pipe_id], copy_len);
            if (copy_len < g_pipe_len[pipe_id]) {
                memmove(g_pipe_buf[pipe_id], g_pipe_buf[pipe_id] + copy_len, g_pipe_len[pipe_id] - copy_len);
            }
            g_pipe_len[pipe_id] = (uint16_t)(g_pipe_len[pipe_id] - copy_len);
            return (int64_t)copy_len;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_SOCKET) {
            if (g_fd_sock_connected[fd] == 0 || len == 0) {
                return -1;
            }
            if (g_fd_sock_type[fd] == QOS_SOCK_STREAM && g_fd_sock_conn_id[fd] < 0 &&
                g_fd_sock_rip[fd] == QOS_NET_IPV4_GATEWAY) {
                uint16_t got = g_fd_sock_stream_len[fd];
                uint32_t remain;
                uint32_t copy_len;
                if (got == 0) {
                    return -1;
                }
                copy_len = got;
                if (copy_len > len) {
                    copy_len = len;
                }
                memcpy(out, g_fd_sock_stream_buf[fd], copy_len);
                remain = (uint32_t)got - copy_len;
                if (remain != 0) {
                    memmove(g_fd_sock_stream_buf[fd], g_fd_sock_stream_buf[fd] + copy_len, remain);
                }
                memset(g_fd_sock_stream_buf[fd] + remain, 0, QOS_NET_MAX_PACKET - remain);
                g_fd_sock_stream_len[fd] = (uint16_t)remain;
                return (int64_t)copy_len;
            }
            if (g_fd_sock_type[fd] == QOS_SOCK_RAW) {
                uint16_t got = g_fd_sock_raw_len[fd];
                if (got == 0 || got > len) {
                    return -1;
                }
                memcpy(out, g_fd_sock_raw_buf[fd], got);
                g_fd_sock_raw_len[fd] = 0;
                g_fd_sock_raw_src_ip[fd] = 0;
                memset(g_fd_sock_raw_buf[fd], 0, sizeof(g_fd_sock_raw_buf[fd]));
                return (int64_t)got;
            }
            if (g_fd_sock_type[fd] == QOS_SOCK_DGRAM) {
                uint32_t src_ip = 0;
                uint16_t src_port = 0;
                if (qos_net_udp_recv == 0 || g_fd_sock_lport[fd] == 0) {
                    return -1;
                }
                return (int64_t)qos_net_udp_recv(g_fd_sock_lport[fd], out, len, &src_ip, &src_port);
            }
            if (qos_net_dequeue_packet == 0) {
                return -1;
            }
            return (int64_t)qos_net_dequeue_packet(out, len);
        }
        return -1;
    }

    if (g_ops[nr] == SYSCALL_OP_WRITE) {
        uint32_t fd = (uint32_t)a0;
        const uint8_t *src = (const uint8_t *)(uintptr_t)a1;
        uint32_t len = (uint32_t)a2;
        if (!fd_valid(fd) || src == 0) {
            return -1;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_PROC) {
            return -1;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_VFS) {
            uint16_t file_id = g_fd_file_id[fd];
            uint32_t offset = (uint32_t)g_fd_offset[fd];
            uint32_t write_len;
            if (file_id >= QOS_FILE_MAX || g_file_used[file_id] == 0) {
                return -1;
            }
            if (offset >= QOS_PIPE_CAP) {
                return -1;
            }
            write_len = len;
            if (write_len > (uint32_t)(QOS_PIPE_CAP - offset)) {
                write_len = (uint32_t)(QOS_PIPE_CAP - offset);
            }
            if (write_len == 0) {
                return 0;
            }
            memcpy(g_file_data[file_id] + offset, src, write_len);
            offset += write_len;
            g_fd_offset[fd] = offset;
            if (offset > g_file_len[file_id]) {
                g_file_len[file_id] = (uint16_t)offset;
            }
            return (int64_t)write_len;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_PIPE_W) {
            uint16_t pipe_id = g_fd_pipe_id[fd];
            if (pipe_id >= QOS_PIPE_MAX || g_pipe_used[pipe_id] == 0 || len > (uint32_t)(QOS_PIPE_CAP - g_pipe_len[pipe_id])) {
                return -1;
            }
            memcpy(g_pipe_buf[pipe_id] + g_pipe_len[pipe_id], src, len);
            g_pipe_len[pipe_id] = (uint16_t)(g_pipe_len[pipe_id] + len);
            return (int64_t)len;
        }

        if (g_fd_kind[fd] == QOS_FD_KIND_SOCKET) {
            if (g_fd_sock_connected[fd] == 0 || len == 0) {
                return -1;
            }
            if (g_fd_sock_type[fd] == QOS_SOCK_STREAM && g_fd_sock_conn_id[fd] < 0 &&
                g_fd_sock_rip[fd] == QOS_NET_IPV4_GATEWAY) {
                if (stream_host_response_prepare(fd, src, len) != 0) {
                    return -1;
                }
                return (int64_t)len;
            }
            if (g_fd_sock_type[fd] == QOS_SOCK_RAW) {
                uint32_t src_ip = 0;
                uint16_t ident = g_fd_sock_lport[fd] != 0 ? g_fd_sock_lport[fd] : 1;
                int got;
                if (qos_net_icmp_echo == 0 || g_fd_sock_rip[fd] == 0 || len > QOS_NET_MAX_PACKET) {
                    return -1;
                }
                got = qos_net_icmp_echo(g_fd_sock_rip[fd], ident, 1, src, len, g_fd_sock_raw_buf[fd],
                                        QOS_NET_MAX_PACKET, &src_ip);
                if (got < 0) {
                    return -1;
                }
                g_fd_sock_raw_len[fd] = (uint16_t)got;
                g_fd_sock_raw_src_ip[fd] = src_ip;
                return (int64_t)len;
            }
            if (g_fd_sock_type[fd] == QOS_SOCK_DGRAM) {
                if (qos_net_udp_send == 0 || g_fd_sock_lport[fd] == 0 || g_fd_sock_rport[fd] == 0 ||
                    g_fd_sock_rip[fd] == 0) {
                    return -1;
                }
                return (int64_t)qos_net_udp_send(g_fd_sock_lport[fd], QOS_NET_IPV4_LOCAL, g_fd_sock_rport[fd], src, len);
            }
            if (qos_net_enqueue_packet == 0) {
                return -1;
            }
            return qos_net_enqueue_packet(src, len) == 0 ? (int64_t)len : -1;
        }
        return -1;
    }

    if (g_ops[nr] == SYSCALL_OP_CLOSE) {
        uint32_t fd = (uint32_t)a0;
        if (!fd_valid(fd)) {
            return -1;
        }
        fd_clear(fd);
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_DUP2) {
        uint32_t oldfd = (uint32_t)a0;
        uint32_t newfd = (uint32_t)a1;
        if (!fd_valid(oldfd) || newfd >= QOS_FD_MAX) {
            return -1;
        }
        if (oldfd == newfd) {
            return (int64_t)newfd;
        }
        if (fd_valid(newfd)) {
            fd_clear(newfd);
        }
        g_fd_used[newfd] = 1;
        g_fd_kind[newfd] = g_fd_kind[oldfd];
        g_fd_sock_bound[newfd] = g_fd_sock_bound[oldfd];
        g_fd_sock_listening[newfd] = g_fd_sock_listening[oldfd];
        g_fd_sock_connected[newfd] = g_fd_sock_connected[oldfd];
        g_fd_sock_pending[newfd] = g_fd_sock_pending[oldfd];
        g_fd_sock_backlog[newfd] = g_fd_sock_backlog[oldfd];
        g_fd_sock_type[newfd] = g_fd_sock_type[oldfd];
        g_fd_sock_lport[newfd] = g_fd_sock_lport[oldfd];
        g_fd_sock_rport[newfd] = g_fd_sock_rport[oldfd];
        g_fd_sock_rip[newfd] = g_fd_sock_rip[oldfd];
        g_fd_sock_conn_id[newfd] = g_fd_sock_conn_id[oldfd];
        g_fd_sock_stream_len[newfd] = g_fd_sock_stream_len[oldfd];
        memcpy(g_fd_sock_stream_buf[newfd], g_fd_sock_stream_buf[oldfd], sizeof(g_fd_sock_stream_buf[newfd]));
        g_fd_sock_raw_len[newfd] = g_fd_sock_raw_len[oldfd];
        g_fd_sock_raw_src_ip[newfd] = g_fd_sock_raw_src_ip[oldfd];
        memcpy(g_fd_sock_raw_buf[newfd], g_fd_sock_raw_buf[oldfd], sizeof(g_fd_sock_raw_buf[newfd]));
        g_fd_sock_pending_rport[newfd] = g_fd_sock_pending_rport[oldfd];
        g_fd_sock_pending_rip[newfd] = g_fd_sock_pending_rip[oldfd];
        g_fd_sock_pending_conn_id[newfd] = g_fd_sock_pending_conn_id[oldfd];
        g_fd_sock_pending_head[newfd] = g_fd_sock_pending_head[oldfd];
        g_fd_sock_pending_tail[newfd] = g_fd_sock_pending_tail[oldfd];
        memcpy(g_fd_sock_pending_q_rport[newfd], g_fd_sock_pending_q_rport[oldfd],
               sizeof(g_fd_sock_pending_q_rport[newfd]));
        memcpy(g_fd_sock_pending_q_rip[newfd], g_fd_sock_pending_q_rip[oldfd],
               sizeof(g_fd_sock_pending_q_rip[newfd]));
        memcpy(g_fd_sock_pending_q_conn_id[newfd], g_fd_sock_pending_q_conn_id[oldfd],
               sizeof(g_fd_sock_pending_q_conn_id[newfd]));
        g_fd_pipe_id[newfd] = g_fd_pipe_id[oldfd];
        g_fd_offset[newfd] = g_fd_offset[oldfd];
        g_fd_file_id[newfd] = g_fd_file_id[oldfd];
        g_fd_proc_kind[newfd] = g_fd_proc_kind[oldfd];
        g_fd_proc_pid[newfd] = g_fd_proc_pid[oldfd];
        return (int64_t)newfd;
    }

    if (g_ops[nr] == SYSCALL_OP_MMAP) {
        uint64_t va = a0;
        uint64_t len = a1;
        uint32_t flags = (uint32_t)a2;
        uint64_t off = 0;
        if (qos_vmm_map == 0 || va == 0 || len == 0) {
            return -1;
        }
        if (flags == 0) {
            flags = QOS_VMM_DEFAULT_FLAGS;
        }
        while (off < len) {
            if (qos_vmm_map(va + off, va + off, flags) != 0) {
                while (off > 0) {
                    off -= QOS_VMM_PAGE_SIZE;
                    (void)qos_vmm_unmap(va + off);
                }
                return -1;
            }
            off += QOS_VMM_PAGE_SIZE;
        }
        return (int64_t)va;
    }

    if (g_ops[nr] == SYSCALL_OP_MUNMAP) {
        uint64_t va = a0;
        uint64_t len = a1;
        uint64_t off = 0;
        if (qos_vmm_unmap == 0 || va == 0 || len == 0) {
            return -1;
        }
        while (off < len) {
            if (qos_vmm_unmap(va + off) != 0) {
                return -1;
            }
            off += QOS_VMM_PAGE_SIZE;
        }
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_YIELD) {
        if (qos_sched_next == 0) {
            return -1;
        }
        return (int64_t)qos_sched_next();
    }

    if (g_ops[nr] == SYSCALL_OP_SLEEP) {
        (void)a0;
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_SOCKET) {
        int fd;
        if (a0 != 2 || (a1 != QOS_SOCK_STREAM && a1 != QOS_SOCK_DGRAM && a1 != QOS_SOCK_RAW)) {
            return -1;
        }
        fd = fd_alloc(QOS_FD_KIND_SOCKET);
        if (fd < 0) {
            return -1;
        }
        g_fd_sock_type[(uint32_t)fd] = (uint8_t)a1;
        return (int64_t)fd;
    }

    if (g_ops[nr] == SYSCALL_OP_BIND) {
        uint32_t fd = (uint32_t)a0;
        uint16_t port = 0;
        uint32_t ip = 0;
        uint8_t sock_type;
        if (!fd_valid(fd) || g_fd_kind[fd] != QOS_FD_KIND_SOCKET || g_fd_sock_bound[fd] != 0) {
            return -1;
        }
        if (parse_sockaddr((const void *)(uintptr_t)a1, (uint32_t)a2, &port, &ip, 1) != 0) {
            return -1;
        }
        sock_type = g_fd_sock_type[fd];
        if (sock_type == QOS_SOCK_RAW) {
            g_fd_sock_bound[fd] = 1;
            g_fd_sock_lport[fd] = port;
            (void)ip;
            return 0;
        }
        if (socket_port_mark(sock_type, port) != 0) {
            return -1;
        }
        if (sock_type == QOS_SOCK_DGRAM) {
            if (qos_net_udp_bind == 0 || qos_net_udp_bind(port) != 0) {
                socket_port_unmark(sock_type, port);
                return -1;
            }
        }
        g_fd_sock_bound[fd] = 1;
        g_fd_sock_lport[fd] = port;
        (void)ip;
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_LISTEN) {
        uint32_t fd = (uint32_t)a0;
        uint32_t backlog = (uint32_t)a1;
        int listener_id;
        if (!fd_valid(fd) || g_fd_kind[fd] != QOS_FD_KIND_SOCKET || g_fd_sock_type[fd] != QOS_SOCK_STREAM ||
            g_fd_sock_bound[fd] == 0 || g_fd_sock_lport[fd] == 0 || a1 == 0 || qos_net_tcp_listen == 0) {
            return -1;
        }
        if (backlog > QOS_SOCK_PENDING_MAX) {
            backlog = QOS_SOCK_PENDING_MAX;
        }
        if (backlog == 0) {
            return -1;
        }
        listener_id = qos_net_tcp_listen(g_fd_sock_lport[fd]);
        if (listener_id < 0) {
            return -1;
        }
        g_fd_sock_listening[fd] = 1;
        g_fd_sock_backlog[fd] = (uint8_t)backlog;
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_ACCEPT) {
        uint32_t fd = (uint32_t)a0;
        int newfd;
        uint16_t peer_port;
        uint32_t peer_ip;
        int16_t peer_conn_id;
        if (!fd_valid(fd) || g_fd_kind[fd] != QOS_FD_KIND_SOCKET || g_fd_sock_type[fd] != QOS_SOCK_STREAM ||
            g_fd_sock_listening[fd] == 0 || g_fd_sock_pending[fd] == 0) {
            return -1;
        }
        newfd = fd_alloc(QOS_FD_KIND_SOCKET);
        if (newfd < 0) {
            return -1;
        }
        if (socket_pending_pop(fd, &peer_port, &peer_ip, &peer_conn_id) != 0) {
            fd_clear((uint32_t)newfd);
            return -1;
        }
        g_fd_sock_bound[(uint32_t)newfd] = 1;
        g_fd_sock_connected[(uint32_t)newfd] = 1;
        g_fd_sock_type[(uint32_t)newfd] = QOS_SOCK_STREAM;
        g_fd_sock_lport[(uint32_t)newfd] = g_fd_sock_lport[fd];
        g_fd_sock_rport[(uint32_t)newfd] = peer_port;
        g_fd_sock_rip[(uint32_t)newfd] = peer_ip;
        g_fd_sock_conn_id[(uint32_t)newfd] = peer_conn_id;
        g_fd_sock_pending_rport[fd] = peer_port;
        g_fd_sock_pending_rip[fd] = peer_ip;
        g_fd_sock_pending_conn_id[fd] = peer_conn_id;
        if (a1 != 0 && a2 != 0) {
            uint8_t *addr = (uint8_t *)(uintptr_t)a1;
            uint32_t *addrlen = (uint32_t *)(uintptr_t)a2;
            if (*addrlen >= 16) {
                memset(addr, 0, 16);
                addr[0] = 2;
                addr[1] = 0;
                addr[2] = (uint8_t)(peer_port >> 8);
                addr[3] = (uint8_t)(peer_port & 0xFF);
                addr[4] = (uint8_t)(peer_ip >> 24);
                addr[5] = (uint8_t)((peer_ip >> 16) & 0xFF);
                addr[6] = (uint8_t)((peer_ip >> 8) & 0xFF);
                addr[7] = (uint8_t)(peer_ip & 0xFF);
                *addrlen = 16;
            } else {
                *addrlen = 0;
            }
        } else if (a2 != 0) {
            uint32_t *addrlen = (uint32_t *)(uintptr_t)a2;
            *addrlen = 0;
        }
        return (int64_t)newfd;
    }

    if (g_ops[nr] == SYSCALL_OP_CONNECT) {
        uint32_t fd = (uint32_t)a0;
        uint16_t remote_port = 0;
        uint32_t remote_ip = 0;
        if (!fd_valid(fd) || g_fd_kind[fd] != QOS_FD_KIND_SOCKET) {
            return -1;
        }
        if (parse_sockaddr((const void *)(uintptr_t)a1, (uint32_t)a2, &remote_port, &remote_ip, 0) != 0) {
            return -1;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_STREAM) {
            uint32_t listener = QOS_FD_MAX;
            uint32_t i;
            uint16_t local_port = g_fd_sock_lport[fd];
            uint8_t listener_backlog = 0;
            int allocated_local = 0;
            int conn_id;
            for (i = 0; i < QOS_FD_MAX; i++) {
                if (i != fd && fd_valid(i) && g_fd_kind[i] == QOS_FD_KIND_SOCKET && g_fd_sock_type[i] == QOS_SOCK_STREAM &&
                    g_fd_sock_listening[i] != 0 && g_fd_sock_lport[i] == remote_port) {
                    listener = i;
                    break;
                }
            }
            if (listener == QOS_FD_MAX) {
                if (remote_ip != QOS_NET_IPV4_GATEWAY) {
                    return -1;
                }
                if (local_port == 0) {
                    local_port = socket_port_alloc(QOS_SOCK_STREAM);
                    if (local_port == 0) {
                        return -1;
                    }
                }
                g_fd_sock_bound[fd] = 1;
                g_fd_sock_lport[fd] = local_port;
                g_fd_sock_connected[fd] = 1;
                g_fd_sock_rport[fd] = remote_port;
                g_fd_sock_rip[fd] = remote_ip;
                g_fd_sock_conn_id[fd] = -1;
                return 0;
            }
            if (qos_net_tcp_connect == 0) {
                return -1;
            }
            listener_backlog = g_fd_sock_backlog[listener];
            if (listener_backlog == 0 || g_fd_sock_pending[listener] >= listener_backlog) {
                return -1;
            }
            if (local_port == 0) {
                local_port = socket_port_alloc(QOS_SOCK_STREAM);
                if (local_port == 0) {
                    return -1;
                }
                allocated_local = 1;
            }
            conn_id = qos_net_tcp_connect(local_port, remote_ip, remote_port);
            if (conn_id < 0) {
                if (allocated_local != 0) {
                    socket_port_unmark(QOS_SOCK_STREAM, local_port);
                }
                return -1;
            }
            if (qos_net_tcp_step != 0 && qos_net_tcp_step(conn_id, QOS_TCP_EVT_RX_SYN_ACK) != 0) {
                if (allocated_local != 0) {
                    socket_port_unmark(QOS_SOCK_STREAM, local_port);
                }
                return -1;
            }
            g_fd_sock_bound[fd] = 1;
            g_fd_sock_lport[fd] = local_port;
            g_fd_sock_connected[fd] = 1;
            g_fd_sock_rport[fd] = remote_port;
            g_fd_sock_rip[fd] = remote_ip;
            g_fd_sock_conn_id[fd] = (int16_t)conn_id;
            if (socket_pending_push(listener, local_port, QOS_NET_IPV4_LOCAL, (int16_t)conn_id) != 0) {
                if (allocated_local != 0) {
                    socket_port_unmark(QOS_SOCK_STREAM, local_port);
                }
                return -1;
            }
            return 0;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_DGRAM) {
            uint16_t local_port = g_fd_sock_lport[fd];
            if (local_port == 0) {
                local_port = socket_port_alloc(QOS_SOCK_DGRAM);
                if (local_port == 0) {
                    return -1;
                }
                if (qos_net_udp_bind == 0 || qos_net_udp_bind(local_port) != 0) {
                    socket_port_unmark(QOS_SOCK_DGRAM, local_port);
                    return -1;
                }
                g_fd_sock_bound[fd] = 1;
                g_fd_sock_lport[fd] = local_port;
            }
            g_fd_sock_connected[fd] = 1;
            g_fd_sock_rport[fd] = remote_port;
            g_fd_sock_rip[fd] = remote_ip;
            return 0;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_RAW) {
            uint16_t local_port = g_fd_sock_lport[fd];
            if (local_port == 0) {
                local_port = 1;
            }
            g_fd_sock_bound[fd] = 1;
            g_fd_sock_lport[fd] = local_port;
            g_fd_sock_connected[fd] = 1;
            g_fd_sock_rport[fd] = remote_port;
            g_fd_sock_rip[fd] = remote_ip;
            return 0;
        }
        return -1;
    }

    if (g_ops[nr] == SYSCALL_OP_SEND) {
        uint32_t fd = (uint32_t)a0;
        const void *buf = (const void *)(uintptr_t)a1;
        uint32_t len = (uint32_t)a2;
        if (!fd_valid(fd) || g_fd_kind[fd] != QOS_FD_KIND_SOCKET || buf == 0 || len == 0) {
            return -1;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_STREAM && g_fd_sock_connected[fd] == 0) {
            return -1;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_RAW) {
            uint32_t src_ip = 0;
            uint16_t ident = g_fd_sock_lport[fd] != 0 ? g_fd_sock_lport[fd] : 1;
            uint16_t remote_port = g_fd_sock_rport[fd];
            uint32_t remote_ip = g_fd_sock_rip[fd];
            int got;
            if (g_fd_sock_connected[fd] == 0) {
                if (parse_sockaddr((const void *)(uintptr_t)a3, 16, &remote_port, &remote_ip, 0) != 0) {
                    return -1;
                }
            }
            if (qos_net_icmp_echo == 0 || remote_ip == 0 || len > QOS_NET_MAX_PACKET) {
                return -1;
            }
            got = qos_net_icmp_echo(remote_ip, ident, 1, buf, len, g_fd_sock_raw_buf[fd], QOS_NET_MAX_PACKET,
                                    &src_ip);
            if (got < 0) {
                return -1;
            }
            g_fd_sock_rport[fd] = remote_port;
            g_fd_sock_rip[fd] = remote_ip;
            g_fd_sock_raw_len[fd] = (uint16_t)got;
            g_fd_sock_raw_src_ip[fd] = src_ip;
            return (int64_t)len;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_DGRAM) {
            uint16_t local_port = g_fd_sock_lport[fd];
            uint16_t remote_port = g_fd_sock_rport[fd];
            uint32_t remote_ip = g_fd_sock_rip[fd];
            if (g_fd_sock_connected[fd] == 0) {
                if (parse_sockaddr((const void *)(uintptr_t)a3, 16, &remote_port, &remote_ip, 0) != 0) {
                    return -1;
                }
            }
            if (local_port == 0) {
                local_port = socket_port_alloc(QOS_SOCK_DGRAM);
                if (local_port == 0) {
                    return -1;
                }
                if (qos_net_udp_bind == 0 || qos_net_udp_bind(local_port) != 0) {
                    socket_port_unmark(QOS_SOCK_DGRAM, local_port);
                    return -1;
                }
                g_fd_sock_bound[fd] = 1;
                g_fd_sock_lport[fd] = local_port;
            }
            if (qos_net_udp_send == 0 || remote_port == 0 || remote_ip == 0) {
                return -1;
            }
            g_fd_sock_rport[fd] = remote_port;
            g_fd_sock_rip[fd] = remote_ip;
            return (int64_t)qos_net_udp_send(local_port, QOS_NET_IPV4_LOCAL, remote_port, buf, len);
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_STREAM && g_fd_sock_conn_id[fd] < 0 &&
            g_fd_sock_rip[fd] == QOS_NET_IPV4_GATEWAY) {
            if (stream_host_response_prepare(fd, (const uint8_t *)buf, len) != 0) {
                return -1;
            }
            return (int64_t)len;
        }
        if (qos_net_enqueue_packet == 0) {
            return -1;
        }
        return qos_net_enqueue_packet(buf, len) == 0 ? (int64_t)len : -1;
    }

    if (g_ops[nr] == SYSCALL_OP_RECV) {
        uint32_t fd = (uint32_t)a0;
        void *buf = (void *)(uintptr_t)a1;
        uint32_t len = (uint32_t)a2;
        if (!fd_valid(fd) || g_fd_kind[fd] != QOS_FD_KIND_SOCKET || buf == 0 || len == 0) {
            return -1;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_STREAM && g_fd_sock_connected[fd] == 0) {
            return -1;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_RAW) {
            uint16_t got = g_fd_sock_raw_len[fd];
            if (got == 0 || got > len) {
                return -1;
            }
            memcpy(buf, g_fd_sock_raw_buf[fd], got);
            write_sockaddr((void *)(uintptr_t)a3, 1, g_fd_sock_raw_src_ip[fd]);
            g_fd_sock_raw_len[fd] = 0;
            g_fd_sock_raw_src_ip[fd] = 0;
            memset(g_fd_sock_raw_buf[fd], 0, sizeof(g_fd_sock_raw_buf[fd]));
            return (int64_t)got;
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_DGRAM) {
            uint32_t src_ip = 0;
            uint16_t src_port = 0;
            if (qos_net_udp_recv == 0 || g_fd_sock_lport[fd] == 0) {
                return -1;
            }
            {
                int got = qos_net_udp_recv(g_fd_sock_lport[fd], buf, len, &src_ip, &src_port);
                if (got < 0) {
                    return -1;
                }
                write_sockaddr((void *)(uintptr_t)a3, src_port, src_ip);
                return (int64_t)got;
            }
        }
        if (g_fd_sock_type[fd] == QOS_SOCK_STREAM && g_fd_sock_conn_id[fd] < 0 &&
            g_fd_sock_rip[fd] == QOS_NET_IPV4_GATEWAY) {
            uint16_t got = g_fd_sock_stream_len[fd];
            uint32_t remain;
            uint32_t copy_len;
            if (got == 0) {
                return -1;
            }
            copy_len = got;
            if (copy_len > len) {
                copy_len = len;
            }
            memcpy(buf, g_fd_sock_stream_buf[fd], copy_len);
            remain = (uint32_t)got - copy_len;
            if (remain != 0) {
                memmove(g_fd_sock_stream_buf[fd], g_fd_sock_stream_buf[fd] + copy_len, remain);
            }
            memset(g_fd_sock_stream_buf[fd] + remain, 0, QOS_NET_MAX_PACKET - remain);
            g_fd_sock_stream_len[fd] = (uint16_t)remain;
            return (int64_t)copy_len;
        }
        if (qos_net_dequeue_packet == 0) {
            return -1;
        }
        return (int64_t)qos_net_dequeue_packet(buf, len);
    }

    if (g_ops[nr] == SYSCALL_OP_GETDENTS) {
        uint32_t fd = (uint32_t)a0;
        uint32_t count;
        if (!fd_valid(fd) || g_fd_kind[fd] != QOS_FD_KIND_VFS || a1 == 0) {
            return -1;
        }
        if (a2 < sizeof(uint32_t) || qos_vfs_count == 0) {
            return -1;
        }
        count = qos_vfs_count();
        *(uint32_t *)(uintptr_t)a1 = count;
        return (int64_t)count;
    }

    if (g_ops[nr] == SYSCALL_OP_LSEEK) {
        uint32_t fd = (uint32_t)a0;
        int64_t offset = (int64_t)a1;
        uint32_t whence = (uint32_t)a2;
        int64_t base;
        int64_t next;
        if (!fd_valid(fd) || (g_fd_kind[fd] != QOS_FD_KIND_VFS && g_fd_kind[fd] != QOS_FD_KIND_PROC)) {
            return -1;
        }
        if (whence == 0) {
            base = 0;
        } else if (whence == 1) {
            base = (int64_t)g_fd_offset[fd];
        } else if (whence == 2) {
            if (g_fd_kind[fd] == QOS_FD_KIND_PROC) {
                base = (int64_t)proc_fd_render(fd, 0, 0);
            } else {
                uint16_t file_id = g_fd_file_id[fd];
                if (file_id >= QOS_FILE_MAX || g_file_used[file_id] == 0) {
                    return -1;
                }
                base = (int64_t)g_file_len[file_id];
            }
        } else {
            return -1;
        }
        if ((offset > 0 && base > INT64_MAX - offset) || (offset < 0 && base < INT64_MIN - offset)) {
            return -1;
        }
        next = base + offset;
        if (next < 0) {
            return -1;
        }
        g_fd_offset[fd] = (uint64_t)next;
        return next;
    }

    if (g_ops[nr] == SYSCALL_OP_PIPE) {
        int pipe_id;
        int rfd;
        int wfd;
        uint32_t *fds = (uint32_t *)(uintptr_t)a0;
        if (fds == 0) {
            return -1;
        }
        pipe_id = pipe_alloc();
        if (pipe_id < 0) {
            return -1;
        }
        rfd = fd_alloc(QOS_FD_KIND_PIPE_R);
        if (rfd < 0) {
            g_pipe_used[(uint32_t)pipe_id] = 0;
            return -1;
        }
        wfd = fd_alloc(QOS_FD_KIND_PIPE_W);
        if (wfd < 0) {
            fd_clear((uint32_t)rfd);
            g_pipe_used[(uint32_t)pipe_id] = 0;
            return -1;
        }
        g_fd_pipe_id[(uint32_t)rfd] = (uint16_t)pipe_id;
        g_fd_pipe_id[(uint32_t)wfd] = (uint16_t)pipe_id;
        fds[0] = (uint32_t)rfd;
        fds[1] = (uint32_t)wfd;
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_KILL) {
        if (qos_proc_signal_send == 0) {
            return -1;
        }
        return qos_proc_signal_send((uint32_t)a0, (uint32_t)a1) == 0 ? 0 : -1;
    }

    if (g_ops[nr] == SYSCALL_OP_SIGPENDING) {
        uint32_t pending = 0;
        if (qos_proc_signal_pending == 0) {
            return -1;
        }
        return qos_proc_signal_pending((uint32_t)a0, &pending) == 0 ? (int64_t)pending : -1;
    }

    if (g_ops[nr] == SYSCALL_OP_SIGACTION) {
        uint64_t old_handler = 0;
        if (qos_proc_signal_get_handler == 0 || qos_proc_signal_set_handler == 0) {
            return -1;
        }
        if (qos_proc_signal_get_handler((uint32_t)a0, (uint32_t)a1, &old_handler) != 0) {
            return -1;
        }
        if (a2 != UINT64_MAX) {
            if (qos_proc_signal_set_handler((uint32_t)a0, (uint32_t)a1, a2) != 0) {
                return -1;
            }
        }
        return (int64_t)old_handler;
    }

    if (g_ops[nr] == SYSCALL_OP_SIGPROCMASK) {
        uint32_t old_mask = 0;
        uint32_t new_mask = 0;
        if (qos_proc_signal_mask == 0 || qos_proc_signal_set_mask == 0) {
            return -1;
        }
        if (qos_proc_signal_mask((uint32_t)a0, &old_mask) != 0) {
            return -1;
        }

        if ((uint32_t)a1 == SIG_BLOCK) {
            new_mask = old_mask | (uint32_t)a2;
        } else if ((uint32_t)a1 == SIG_UNBLOCK) {
            new_mask = old_mask & ~(uint32_t)a2;
        } else if ((uint32_t)a1 == SIG_SETMASK) {
            new_mask = (uint32_t)a2;
        } else {
            return -1;
        }

        if (qos_proc_signal_set_mask((uint32_t)a0, new_mask) != 0) {
            return -1;
        }
        return (int64_t)old_mask;
    }

    if (g_ops[nr] == SYSCALL_OP_SIGALTSTACK) {
        qos_sigaltstack_t old_altstack;
        if (qos_proc_signal_altstack_get == 0 || qos_proc_signal_altstack_set == 0) {
            return -1;
        }
        if (qos_proc_signal_altstack_get((uint32_t)a0, &old_altstack) != 0) {
            return -1;
        }
        if (a1 != UINT64_MAX) {
            if (qos_proc_signal_altstack_set((uint32_t)a0, a1, a2, (uint32_t)a3) != 0) {
                return -1;
            }
        }
        return (int64_t)old_altstack.flags;
    }

    if (g_ops[nr] == SYSCALL_OP_SIGSUSPEND) {
        uint32_t old_mask = 0;
        int32_t delivered = -1;
        if (qos_proc_signal_mask == 0 || qos_proc_signal_set_mask == 0 || qos_proc_signal_next == 0) {
            return -1;
        }
        if (qos_proc_signal_mask((uint32_t)a0, &old_mask) != 0) {
            return -1;
        }
        if (qos_proc_signal_set_mask((uint32_t)a0, (uint32_t)a1) != 0) {
            return -1;
        }
        delivered = qos_proc_signal_next((uint32_t)a0);
        if (qos_proc_signal_set_mask((uint32_t)a0, old_mask) != 0) {
            return -1;
        }
        return delivered < 0 ? -1 : (int64_t)delivered;
    }

    if (g_ops[nr] == SYSCALL_OP_FORK) {
        if (qos_proc_fork == 0 || qos_proc_fork((uint32_t)a0, (uint32_t)a1) != 0) {
            return -1;
        }
        return (int64_t)(uint32_t)a1;
    }

    if (g_ops[nr] == SYSCALL_OP_EXEC) {
        if (qos_proc_exec_signal_reset == 0) {
            return -1;
        }
        return qos_proc_exec_signal_reset((uint32_t)a0) == 0 ? 0 : -1;
    }

    if (g_ops[nr] == SYSCALL_OP_EXIT) {
        if (qos_proc_exit == 0) {
            return -1;
        }
        return qos_proc_exit((uint32_t)a0, (int32_t)a1) == 0 ? 0 : -1;
    }

    if (g_ops[nr] == SYSCALL_OP_GETPID) {
        if (qos_proc_alive == 0 || qos_proc_alive((uint32_t)a0) == 0) {
            return -1;
        }
        return (int64_t)(uint32_t)a0;
    }

    if (g_ops[nr] == SYSCALL_OP_WAITPID) {
        int32_t *statusp = (int32_t *)(uintptr_t)a2;
        if (qos_proc_wait == 0) {
            return -1;
        }
        return (int64_t)qos_proc_wait((uint32_t)a0, (int32_t)a1, statusp, (uint32_t)a3);
    }

    if (g_ops[nr] == SYSCALL_OP_STAT) {
        const char *path = (const char *)(uintptr_t)a0;
        uint8_t proc_kind = QOS_PROC_FD_NONE;
        uint32_t proc_pid = 0;
        uint64_t type = 1;
        if (qos_vfs_exists == 0 || path == 0) {
            return -1;
        }
        if (qos_vfs_exists(path) == 0) {
            if (proc_path_kind(path, &proc_kind, &proc_pid) != 0) {
                return -1;
            }
            if (proc_kind == QOS_PROC_FD_STATUS && (qos_proc_alive == 0 || qos_proc_alive(proc_pid) == 0)) {
                return -1;
            }
            type = 2;
        }
        if (a1 != 0) {
            uint64_t *out = (uint64_t *)(uintptr_t)a1;
            out[0] = (uint64_t)strlen(path);
            out[1] = type;
        }
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_MKDIR) {
        const char *path = (const char *)(uintptr_t)a0;
        if (qos_vfs_create == 0 || path == 0) {
            return -1;
        }
        if (qos_vfs_fs_kind != 0 && qos_vfs_fs_kind(path) == QOS_VFS_FS_PROCFS) {
            return -1;
        }
        return qos_vfs_create(path) == 0 ? 0 : -1;
    }

    if (g_ops[nr] == SYSCALL_OP_UNLINK) {
        const char *path = (const char *)(uintptr_t)a0;
        if (qos_vfs_remove == 0 || path == 0) {
            return -1;
        }
        if (qos_vfs_fs_kind != 0 && qos_vfs_fs_kind(path) == QOS_VFS_FS_PROCFS) {
            return -1;
        }
        if (qos_vfs_remove(path) != 0) {
            return -1;
        }
        file_unlink_path(path);
        return 0;
    }

    if (g_ops[nr] == SYSCALL_OP_CHDIR) {
        const char *path = (const char *)(uintptr_t)a1;
        if (qos_proc_cwd_set == 0 || qos_vfs_exists == 0 || path == 0) {
            return -1;
        }
        if (qos_vfs_exists(path) == 0) {
            return -1;
        }
        return qos_proc_cwd_set((uint32_t)a0, path) == 0 ? 0 : -1;
    }

    if (g_ops[nr] == SYSCALL_OP_GETCWD) {
        char tmp[QOS_PROC_CWD_MAX];
        if (qos_proc_cwd_get == 0) {
            return -1;
        }
        if (a1 == 0) {
            return (int64_t)qos_proc_cwd_get((uint32_t)a0, tmp, sizeof(tmp));
        }
        return (int64_t)qos_proc_cwd_get((uint32_t)a0, (char *)(uintptr_t)a1, (uint32_t)a2);
    }

    if (g_ops[nr] == SYSCALL_OP_SIGRETURN) {
        if ((uint32_t)a2 == 1u) {
            const qos_sigframe_t *frame = (const qos_sigframe_t *)(uintptr_t)a1;
            if (qos_proc_signal_sigreturn == 0 || frame == 0) {
                return -1;
            }
            return qos_proc_signal_sigreturn((uint32_t)a0, frame) == 0 ? 0 : -1;
        }
        if (qos_proc_signal_set_mask == 0) {
            return -1;
        }
        return qos_proc_signal_set_mask((uint32_t)a0, (uint32_t)a1) == 0 ? 0 : -1;
    }

    return -1;
}

uint32_t qos_syscall_count(void) {
    return g_count;
}

void syscall_init(void) {
    qos_syscall_reset();
    syscall_register_default_signal_ops();
    syscall_register_default_queries();
    qos_kernel_state_mark(QOS_INIT_SYSCALL);
}
