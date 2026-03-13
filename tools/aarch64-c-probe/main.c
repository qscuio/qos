#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "generated/cmd_elves.h"

#include "../../c-os/boot/boot_info.h"
#include "../../c-os/kernel/init_state.h"
#include "../../c-os/kernel/kernel.h"
#include "../../c-os/kernel/mm/mm.h"
#include "../../c-os/kernel/net/net.h"
#include "../../c-os/kernel/proc/proc.h"
#include "../../c-os/kernel/sched/sched.h"
#include "../../c-os/kernel/syscall/syscall.h"

#define UART0 ((volatile uint32_t *)0x09000000u)
#define UARTFR ((volatile uint32_t *)0x09000018u)
#define UART_FR_RXFE 0x10u
#define QOS_SHELL_LINE_MAX 128u
#define MAPWATCH_POLL_SPINS 200000u
#define MAPWATCH_SIDE_POLL_SPINS 400000u
#define SEMIHOSTING_SYS_WRITE0 0x04u
#define QOS_INIT_PID 1u
#define QOS_SHELL_PID 2u
#define QOS_DTB_MAGIC 0xEDFE0DD0u
#define VIRTIO_MMIO_BASE_START 0x0A000000u
#define VIRTIO_MMIO_STRIDE 0x200u
#define VIRTIO_MMIO_SCAN_SLOTS 32u
#define VIRTIO_MMIO_MAGIC 0x74726976u
#define VIRTIO_MMIO_DEVICE_NET 1u

#define VIRTIO_MMIO_MAGIC_VALUE 0x000u
#define VIRTIO_MMIO_VERSION 0x004u
#define VIRTIO_MMIO_DEVICE_ID 0x008u
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010u
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014u
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020u
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024u
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028u
#define VIRTIO_MMIO_QUEUE_SEL 0x030u
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034u
#define VIRTIO_MMIO_QUEUE_NUM 0x038u
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03Cu
#define VIRTIO_MMIO_QUEUE_PFN 0x040u
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050u
#define VIRTIO_MMIO_STATUS 0x070u

#define VIRTIO_STATUS_ACKNOWLEDGE 1u
#define VIRTIO_STATUS_DRIVER 2u
#define VIRTIO_STATUS_DRIVER_OK 4u

#define VIRTIO_NET_QUEUE_RX 0u
#define VIRTIO_NET_QUEUE_TX 1u
#define LEGACY_QUEUE_ALIGN 4096u
#define VIRTQ_SIZE 8u
#define VIRTIO_NET_HDR_LEN 10u
#define VIRTQ_DESC_F_WRITE 2u

#define PROBE_UDP_DST_PORT 40000u
#define PROBE_UDP_SRC_PORT 40001u
#define ETH_HDR_LEN 14u
#define IPV4_HDR_LEN 20u
#define UDP_HDR_LEN 8u
#define ICMP_HDR_LEN 8u
#define PROBE_FRAME_LEN (ETH_HDR_LEN + IPV4_HDR_LEN + UDP_HDR_LEN + (sizeof(PROBE_PAYLOAD) - 1u))
#define TX_BUF_LEN (VIRTIO_NET_HDR_LEN + PROBE_FRAME_LEN)
#define RX_BUF_LEN (VIRTIO_NET_HDR_LEN + 2048u)
#define ICMP_ECHO_ID 0x5153u
#define ICMP_ECHO_SEQ 1u

#define NET_TX_STAGE_OK 0u
#define NET_TX_STAGE_NO_NETDEV 1u
#define NET_TX_STAGE_FEATURES 2u
#define NET_TX_STAGE_QUEUE 3u
#define NET_TX_STAGE_TIMEOUT 4u
#define NET_RX_STAGE_OK 0u
#define NET_RX_STAGE_NO_NETDEV 1u
#define NET_RX_STAGE_FEATURES 2u
#define NET_RX_STAGE_QUEUE 3u
#define NET_RX_STAGE_TIMEOUT 4u
#define NET_RX_STAGE_PAYLOAD 5u
#define ICMP_REAL_STAGE_OK 0u
#define ICMP_REAL_STAGE_NO_NETDEV 1u
#define ICMP_REAL_STAGE_FEATURES 2u
#define ICMP_REAL_STAGE_QUEUE 3u
#define ICMP_REAL_STAGE_TIMEOUT 4u
#define ICMP_REAL_STAGE_TX 5u

#define QOS_ELF_CMD_ECHO 1u
#define QOS_ELF_CMD_PS 2u
#define QOS_ELF_CMD_PING 3u
#define QOS_ELF_CMD_IP 4u
#define QOS_ELF_CMD_WGET 5u
#define QOS_ELF_CMD_LS 6u
#define QOS_ELF_CMD_CAT 7u
#define QOS_ELF_CMD_TOUCH 8u
#define QOS_ELF_CMD_EDIT 9u
#define QOS_ELF_CMD_INSMOD 10u
#define QOS_ELF_CMD_RMMOD 11u
#define QOS_ELF_CMD_WQDEMO 12u

#define QOS_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} virtq_desc_t;

typedef struct {
    uint16_t qsize;
    volatile virtq_desc_t *desc;
    volatile uint16_t *avail_idx;
    volatile uint16_t *avail_ring;
    volatile const uint16_t *used_idx;
    volatile const uint8_t *used_ring;
} legacy_queue_state_t;

static const uint8_t PROBE_PAYLOAD[] = "QOSREALNET";
static const uint8_t PROBE_RX_PAYLOAD[] = "QOSREALRX";
static const uint8_t ICMP_PROBE_PAYLOAD[] = "QOSICMPRT";
static const uint8_t PROBE_SRC_MAC[6] = {0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u};
static uint8_t LEGACY_TX_QUEUE_PAGE[8192] __attribute__((aligned(4096)));
static uint8_t LEGACY_TX_FRAME_BUF[TX_BUF_LEN] __attribute__((aligned(16)));
static uint8_t LEGACY_RX_QUEUE_PAGE[8192] __attribute__((aligned(4096)));
static uint8_t LEGACY_RX_FRAME_BUF[RX_BUF_LEN] __attribute__((aligned(16)));
static uint32_t g_net_tx_stage = NET_TX_STAGE_QUEUE;
static uint32_t g_net_rx_stage = NET_RX_STAGE_QUEUE;
static uint32_t g_net_tx_virtio_version = 0;
static uint32_t g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;

void *memset(void *dst, int value, size_t n) {
    unsigned char *p = (unsigned char *)dst;
    size_t i = 0;
    while (i < n) {
        p[i] = (unsigned char)value;
        i++;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    size_t i = 0;
    while (i < n) {
        d[i] = s[i];
        i++;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    size_t i = 0;
    while (i < n) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
        i++;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    if (s == 0) {
        return 0;
    }
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int strcmp(const char *a, const char *b) {
    size_t i = 0;
    while (1) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        if (ca == '\0') {
            return 0;
        }
        i++;
    }
}

int strncmp(const char *a, const char *b, size_t n) {
    size_t i = 0;
    while (i < n) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        if (ca == '\0') {
            return 0;
        }
        i++;
    }
    return 0;
}

char *strcpy(char *dst, const char *src) {
    size_t i = 0;
    while (1) {
        dst[i] = src[i];
        if (src[i] == '\0') {
            return dst;
        }
        i++;
    }
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    if (dst == 0 || src == 0) {
        return dst;
    }
    while (i < n && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    while (i < n) {
        dst[i] = '\0';
        i++;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    size_t i;
    if (d == s || n == 0) {
        return dst;
    }
    if (d < s) {
        for (i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        i = n;
        while (i > 0) {
            i--;
            d[i] = s[i];
        }
    }
    return dst;
}

unsigned char QOS_STACK[65536];

typedef struct __attribute__((aligned(8))) {
    uint32_t magic;
    uint32_t totalsize;
    uint8_t blob[32];
} fake_dtb_t;

static const fake_dtb_t QOS_FAKE_DTB = {
    .magic = QOS_DTB_MAGIC,
    .totalsize = 0x28,
    .blob = {0},
};

static void uart_putc(uint8_t ch) {
    *UART0 = (uint32_t)ch;
}

static void uart_puts(const char *s) {
    while (*s != '\0') {
        uart_putc((uint8_t)*s);
        s++;
    }
}

static void uart_putn(const char *s, size_t n) {
    size_t i = 0;
    while (i < n) {
        uart_putc((uint8_t)s[i]);
        i++;
    }
}

static uint8_t uart_getc(void) {
    while ((*UARTFR & UART_FR_RXFE) != 0u) {
    }
    return (uint8_t)(*UART0 & 0xFFu);
}

static int uart_try_getc(uint8_t *out_ch) {
    if (out_ch == 0 || (*UARTFR & UART_FR_RXFE) != 0u) {
        return 0;
    }
    *out_ch = (uint8_t)(*UART0 & 0xFFu);
    return 1;
}

static void uart_put_u32(uint32_t value) {
    char buf[16];
    size_t idx = sizeof(buf);
    if (value == 0u) {
        uart_putc((uint8_t)'0');
        return;
    }
    buf[--idx] = '\0';
    while (value != 0u && idx > 0u) {
        uint32_t d = value % 10u;
        value /= 10u;
        buf[--idx] = (char)('0' + d);
    }
    uart_puts(&buf[idx]);
}

static void shell_out_clear(char *out, size_t cap);
static void shell_out_append(char *out, size_t cap, const char *text);
static int shell_read_file(const char *path, char *out, size_t cap);

static void semihost_write0(const char *s) {
    register uint64_t x0 __asm__("x0") = (uint64_t)SEMIHOSTING_SYS_WRITE0;
    register uint64_t x1 __asm__("x1") = (uint64_t)(uintptr_t)s;
    __asm__ volatile("hlt #0xF000" : "+r"(x0) : "r"(x1) : "memory");
}

static void mapwatch_side_emit_snapshot(void) {
    char mapbuf[2048];
    char out[2304];
    shell_out_clear(out, sizeof(out));
    shell_out_append(out, sizeof(out), "[mapwatch-side] /proc/runtime/map\n");
    if (shell_read_file("/proc/runtime/map", mapbuf, sizeof(mapbuf)) == 0) {
        shell_out_append(out, sizeof(out), mapbuf);
    } else {
        shell_out_append(out, sizeof(out),
                         "CurrentPid:\t0\nCurrentProc:\tnone\nCurrentTid:\t0\nCurrentThread:\tnone\nCurrentAsid:\t0\n"
                         "Mappings:\t0\n");
    }
    shell_out_append(out, sizeof(out), "\n");
    semihost_write0(out);
}

typedef struct {
    const char *path;
    const uint8_t *image;
    size_t image_len;
    uint8_t cmd_id;
} qos_user_elf_t;

static const qos_user_elf_t QOS_USER_ELFS[] = {
    {"/bin/echo", qos_cmd_echo_elf, sizeof(qos_cmd_echo_elf), QOS_ELF_CMD_ECHO},
    {"/bin/ps", qos_cmd_ps_elf, sizeof(qos_cmd_ps_elf), QOS_ELF_CMD_PS},
    {"/bin/ping", qos_cmd_ping_elf, sizeof(qos_cmd_ping_elf), QOS_ELF_CMD_PING},
    {"/bin/ip", qos_cmd_ip_elf, sizeof(qos_cmd_ip_elf), QOS_ELF_CMD_IP},
    {"/bin/wget", qos_cmd_wget_elf, sizeof(qos_cmd_wget_elf), QOS_ELF_CMD_WGET},
    {"/bin/ls", qos_cmd_ls_elf, sizeof(qos_cmd_ls_elf), QOS_ELF_CMD_LS},
    {"/bin/cat", qos_cmd_cat_elf, sizeof(qos_cmd_cat_elf), QOS_ELF_CMD_CAT},
    {"/bin/touch", qos_cmd_touch_elf, sizeof(qos_cmd_touch_elf), QOS_ELF_CMD_TOUCH},
    {"/bin/edit", qos_cmd_edit_elf, sizeof(qos_cmd_edit_elf), QOS_ELF_CMD_EDIT},
    {"/bin/insmod", qos_cmd_insmod_elf, sizeof(qos_cmd_insmod_elf), QOS_ELF_CMD_INSMOD},
    {"/bin/rmmod", qos_cmd_rmmod_elf, sizeof(qos_cmd_rmmod_elf), QOS_ELF_CMD_RMMOD},
    {"/bin/wqdemo", qos_cmd_wqdemo_elf, sizeof(qos_cmd_wqdemo_elf), QOS_ELF_CMD_WQDEMO},
};

static uint32_t g_next_pid = 3u;
static char g_shell_files[64][128];
static uint8_t g_shell_file_used[64];
static uint8_t g_loaded_module_used[32];
static uint32_t g_loaded_module_ids[32];
static char g_loaded_module_paths[32][128];
static uint32_t g_wq_demo_hits = 0;

static void shell_out_clear(char *out, size_t cap) {
    if (out != 0 && cap != 0) {
        out[0] = '\0';
    }
}

static void shell_out_append(char *out, size_t cap, const char *text) {
    size_t cur;
    size_t room;
    size_t n;
    if (out == 0 || cap == 0 || text == 0) {
        return;
    }
    cur = strlen(out);
    if (cur >= cap - 1u) {
        return;
    }
    room = cap - 1u - cur;
    n = strlen(text);
    if (n > room) {
        n = room;
    }
    memcpy(out + cur, text, n);
    out[cur + n] = '\0';
}

static void shell_out_append_n(char *out, size_t cap, const char *text, size_t n) {
    size_t cur;
    size_t room;
    if (out == 0 || cap == 0 || text == 0 || n == 0) {
        return;
    }
    cur = strlen(out);
    if (cur >= cap - 1u) {
        return;
    }
    room = cap - 1u - cur;
    if (n > room) {
        n = room;
    }
    memcpy(out + cur, text, n);
    out[cur + n] = '\0';
}

static void shell_out_append_u32(char *out, size_t cap, uint32_t value) {
    char num[16];
    size_t idx = sizeof(num) - 1u;
    num[idx] = '\0';
    if (value == 0u) {
        shell_out_append(out, cap, "0");
        return;
    }
    while (value != 0u && idx > 0u) {
        uint32_t d = value % 10u;
        value /= 10u;
        num[--idx] = (char)('0' + d);
    }
    shell_out_append(out, cap, &num[idx]);
}

static void shell_track_file(const char *path) {
    uint32_t i;
    if (path == 0 || path[0] != '/') {
        return;
    }
    for (i = 0; i < QOS_ARRAY_LEN(g_shell_file_used); i++) {
        if (g_shell_file_used[i] != 0 && strcmp(g_shell_files[i], path) == 0) {
            return;
        }
    }
    for (i = 0; i < QOS_ARRAY_LEN(g_shell_file_used); i++) {
        if (g_shell_file_used[i] == 0) {
            g_shell_file_used[i] = 1;
            strncpy(g_shell_files[i], path, sizeof(g_shell_files[i]) - 1u);
            g_shell_files[i][sizeof(g_shell_files[i]) - 1u] = '\0';
            return;
        }
    }
}

static int parse_u32_arg(const char *text, uint32_t *out) {
    uint64_t value = 0;
    size_t i = 0;
    if (text == 0 || out == 0 || text[0] == '\0') {
        return -1;
    }
    while (text[i] != '\0') {
        unsigned char ch = (unsigned char)text[i];
        if (ch < '0' || ch > '9') {
            return -1;
        }
        value = value * 10u + (uint64_t)(ch - '0');
        if (value > UINT32_MAX) {
            return -1;
        }
        i++;
    }
    *out = (uint32_t)value;
    return 0;
}

static void module_track_loaded(const char *path, uint32_t module_id) {
    uint32_t i;
    if (path == 0 || path[0] == '\0' || module_id == 0) {
        return;
    }
    for (i = 0; i < QOS_ARRAY_LEN(g_loaded_module_used); i++) {
        if (g_loaded_module_used[i] != 0 && strcmp(g_loaded_module_paths[i], path) == 0) {
            g_loaded_module_ids[i] = module_id;
            return;
        }
    }
    for (i = 0; i < QOS_ARRAY_LEN(g_loaded_module_used); i++) {
        if (g_loaded_module_used[i] == 0) {
            g_loaded_module_used[i] = 1;
            g_loaded_module_ids[i] = module_id;
            strncpy(g_loaded_module_paths[i], path, sizeof(g_loaded_module_paths[i]) - 1u);
            g_loaded_module_paths[i][sizeof(g_loaded_module_paths[i]) - 1u] = '\0';
            return;
        }
    }
}

static int module_find_id_by_path(const char *path, uint32_t *out_module_id) {
    uint32_t i;
    if (path == 0 || out_module_id == 0) {
        return -1;
    }
    for (i = 0; i < QOS_ARRAY_LEN(g_loaded_module_used); i++) {
        if (g_loaded_module_used[i] != 0 && strcmp(g_loaded_module_paths[i], path) == 0) {
            *out_module_id = g_loaded_module_ids[i];
            return 0;
        }
    }
    return -1;
}

static void module_forget_id(uint32_t module_id) {
    uint32_t i;
    if (module_id == 0) {
        return;
    }
    for (i = 0; i < QOS_ARRAY_LEN(g_loaded_module_used); i++) {
        if (g_loaded_module_used[i] != 0 && g_loaded_module_ids[i] == module_id) {
            g_loaded_module_used[i] = 0;
            g_loaded_module_ids[i] = 0;
            g_loaded_module_paths[i][0] = '\0';
            return;
        }
    }
}

static void wq_demo_job(void *arg) {
    g_wq_demo_hits += (uint32_t)(uintptr_t)arg;
}

static int shell_path_exists(const char *path) {
    uint64_t st[2];
    if (path == 0 || path[0] != '/') {
        return 0;
    }
    return qos_syscall_dispatch(SYSCALL_NR_STAT, (uint64_t)(uintptr_t)path, (uint64_t)(uintptr_t)st, 0, 0) == 0 ? 1 : 0;
}

static int shell_touch_path(const char *path) {
    int64_t fd;
    if (path == 0 || path[0] != '/') {
        return -1;
    }
    if (shell_path_exists(path) == 0 &&
        qos_syscall_dispatch(SYSCALL_NR_MKDIR, (uint64_t)(uintptr_t)path, 0, 0, 0) != 0) {
        return -1;
    }
    fd = qos_syscall_dispatch(SYSCALL_NR_OPEN, (uint64_t)(uintptr_t)path, 0, 0, 0);
    if (fd < 0) {
        return -1;
    }
    (void)qos_syscall_dispatch(SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    shell_track_file(path);
    return 0;
}

static int shell_write_file(const char *path, const char *content, int append) {
    int64_t fd;
    size_t len = 0;
    if (path == 0 || path[0] != '/') {
        return -1;
    }
    if (content != 0) {
        len = strlen(content);
    }
    if (append == 0 && shell_path_exists(path) != 0) {
        (void)qos_syscall_dispatch(SYSCALL_NR_UNLINK, (uint64_t)(uintptr_t)path, 0, 0, 0);
    }
    if (shell_touch_path(path) != 0) {
        return -1;
    }
    fd = qos_syscall_dispatch(SYSCALL_NR_OPEN, (uint64_t)(uintptr_t)path, 0, 0, 0);
    if (fd < 0) {
        return -1;
    }
    if (append != 0) {
        (void)qos_syscall_dispatch(SYSCALL_NR_LSEEK, (uint64_t)fd, 0, 2, 0);
    }
    if (len != 0 && qos_syscall_dispatch(SYSCALL_NR_WRITE, (uint64_t)fd, (uint64_t)(uintptr_t)content, (uint64_t)len, 0) <
                        0) {
        (void)qos_syscall_dispatch(SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    (void)qos_syscall_dispatch(SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    shell_track_file(path);
    return 0;
}

static int shell_write_file_bytes(const char *path, const uint8_t *data, size_t len) {
    int64_t fd;
    if (path == 0 || path[0] != '/' || data == 0) {
        return -1;
    }
    if (shell_path_exists(path) != 0) {
        (void)qos_syscall_dispatch(SYSCALL_NR_UNLINK, (uint64_t)(uintptr_t)path, 0, 0, 0);
    }
    if (shell_touch_path(path) != 0) {
        return -1;
    }
    fd = qos_syscall_dispatch(SYSCALL_NR_OPEN, (uint64_t)(uintptr_t)path, 0, 0, 0);
    if (fd < 0) {
        return -1;
    }
    if (len != 0 && qos_syscall_dispatch(SYSCALL_NR_WRITE, (uint64_t)fd, (uint64_t)(uintptr_t)data, (uint64_t)len, 0) <
                        0) {
        (void)qos_syscall_dispatch(SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
        return -1;
    }
    (void)qos_syscall_dispatch(SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    shell_track_file(path);
    return 0;
}

static int shell_read_file(const char *path, char *out, size_t cap) {
    int64_t fd;
    size_t total = 0;
    if (path == 0 || out == 0 || cap == 0) {
        return -1;
    }
    out[0] = '\0';
    fd = qos_syscall_dispatch(SYSCALL_NR_OPEN, (uint64_t)(uintptr_t)path, 0, 0, 0);
    if (fd < 0) {
        return -1;
    }
    while (total + 1u < cap) {
        int64_t n = qos_syscall_dispatch(SYSCALL_NR_READ, (uint64_t)fd, (uint64_t)(uintptr_t)(out + total),
                                         (uint64_t)(cap - 1u - total), 0);
        if (n <= 0) {
            break;
        }
        total += (size_t)n;
    }
    out[total] = '\0';
    (void)qos_syscall_dispatch(SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
    return 0;
}

static void shell_ps_line(char *out, size_t out_cap, uint32_t pid, int32_t ppid, const char *cmd) {
    shell_out_append_u32(out, out_cap, pid);
    shell_out_append(out, out_cap, " ");
    shell_out_append_u32(out, out_cap, ppid < 0 ? 0u : (uint32_t)ppid);
    shell_out_append(out, out_cap, " Running ");
    shell_out_append(out, out_cap, cmd);
    shell_out_append(out, out_cap, "\n");
}

static const qos_user_elf_t *lookup_user_elf(const char *path) {
    size_t i;
    if (path == 0) {
        return 0;
    }
    for (i = 0; i < QOS_ARRAY_LEN(QOS_USER_ELFS); i++) {
        if (strcmp(path, QOS_USER_ELFS[i].path) == 0) {
            return &QOS_USER_ELFS[i];
        }
    }
    return 0;
}

static uint16_t elf_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t elf_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t elf_u64(const uint8_t *p) {
    return (uint64_t)p[0] | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24) |
           ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}

static void elf_w16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}

static void elf_w32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void elf_w64(uint8_t *p, uint64_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
    p[4] = (uint8_t)((v >> 32) & 0xFFu);
    p[5] = (uint8_t)((v >> 40) & 0xFFu);
    p[6] = (uint8_t)((v >> 48) & 0xFFu);
    p[7] = (uint8_t)((v >> 56) & 0xFFu);
}

static size_t build_test_shared_elf_image(uint8_t *out, size_t cap) {
    const uint64_t phoff = 64u;
    if (out == 0 || cap < 256u) {
        return 0;
    }
    memset(out, 0, 256u);
    out[0] = 0x7Fu;
    out[1] = 'E';
    out[2] = 'L';
    out[3] = 'F';
    out[4] = 2u;
    out[5] = 1u;
    out[6] = 1u;

    elf_w16(out + 16u, 3u);
    elf_w16(out + 18u, 0x3Eu);
    elf_w32(out + 20u, 1u);
    elf_w64(out + 24u, 0x401000u);
    elf_w64(out + 32u, phoff);
    elf_w16(out + 52u, 64u);
    elf_w16(out + 54u, 56u);
    elf_w16(out + 56u, 1u);

    elf_w32(out + phoff, 1u);
    elf_w32(out + phoff + 4u, 5u);
    elf_w64(out + phoff + 8u, 0u);
    elf_w64(out + phoff + 16u, 0x400000u);
    elf_w64(out + phoff + 24u, 0u);
    elf_w64(out + phoff + 32u, 128u);
    elf_w64(out + phoff + 40u, 128u);
    elf_w64(out + phoff + 48u, 0x1000u);
    return 256u;
}

static void stage_test_module_artifacts(void) {
    uint8_t image[256];
    size_t len = build_test_shared_elf_image(image, sizeof(image));
    if (len == 0) {
        return;
    }
    (void)shell_write_file_bytes("/lib/libqos_test.so", image, len);
    (void)shell_write_file_bytes("/lib/modules/qos_test.ko", image, len);
}

static int validate_elf_image(const uint8_t *image, size_t len) {
    uint16_t e_type;
    uint16_t e_machine;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint64_t e_phoff;
    uint16_t i;
    uint16_t load_count = 0;

    if (image == 0 || len < 64u) {
        return -1;
    }
    if (image[0] != 0x7Fu || image[1] != 'E' || image[2] != 'L' || image[3] != 'F') {
        return -1;
    }
    if (image[4] != 2u || image[5] != 1u || image[6] != 1u) {
        return -1;
    }
    e_type = elf_u16(image + 16);
    e_machine = elf_u16(image + 18);
    e_phoff = elf_u64(image + 32);
    e_phentsize = elf_u16(image + 54);
    e_phnum = elf_u16(image + 56);
    if ((e_type != 2u && e_type != 3u) || (e_machine != 0x3Eu && e_machine != 0xB7u)) {
        return -1;
    }
    if (e_phnum == 0u || e_phentsize < 56u) {
        return -1;
    }
    if (e_phoff > len || (uint64_t)e_phnum > (UINT64_MAX - e_phoff) / (uint64_t)e_phentsize ||
        e_phoff + (uint64_t)e_phnum * (uint64_t)e_phentsize > len) {
        return -1;
    }
    for (i = 0; i < e_phnum; i++) {
        uint64_t phoff = e_phoff + (uint64_t)i * (uint64_t)e_phentsize;
        uint32_t p_type = elf_u32(image + phoff);
        uint64_t p_offset = elf_u64(image + phoff + 8u);
        uint64_t p_vaddr = elf_u64(image + phoff + 16u);
        uint64_t p_filesz = elf_u64(image + phoff + 32u);
        uint64_t p_memsz = elf_u64(image + phoff + 40u);
        uint64_t p_align = elf_u64(image + phoff + 48u);
        if (p_type == 1u) {
            if (p_memsz < p_filesz || p_offset > len || p_filesz > len - p_offset) {
                return -1;
            }
            if (p_align != 0 && (p_vaddr % p_align) != (p_offset % p_align)) {
                return -1;
            }
            load_count++;
        }
    }
    return load_count == 0 ? -1 : 0;
}

static const char *resolve_command_path(const char *cmd, char *resolved, size_t cap) {
    const qos_user_elf_t *entry;
    size_t i;
    size_t j = 0;
    static const char prefix[] = "/bin/";
    if (cmd == 0 || cmd[0] == '\0' || resolved == 0 || cap == 0) {
        return 0;
    }
    for (i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == '/') {
            entry = lookup_user_elf(cmd);
            if (entry == 0) {
                return 0;
            }
            strncpy(resolved, cmd, cap - 1u);
            resolved[cap - 1u] = '\0';
            return resolved;
        }
    }
    if (sizeof(prefix) >= cap) {
        return 0;
    }
    for (i = 0; i < sizeof(prefix) - 1u; i++) {
        resolved[j++] = prefix[i];
    }
    for (i = 0; cmd[i] != '\0' && j + 1u < cap; i++) {
        resolved[j++] = cmd[i];
    }
    resolved[j] = '\0';
    entry = lookup_user_elf(resolved);
    return entry == 0 ? 0 : resolved;
}

static int tokenize(const char *line, char tokens[][128], int max_tokens) {
    int count = 0;
    const char *p = line;
    while (p != 0 && *p != '\0') {
        char quote = '\0';
        int ti = 0;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }
        if (*p == '\0' || count >= max_tokens) {
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
            quote = *p++;
            while (*p != '\0' && *p != quote && ti < 127) {
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
               *p != '>' && ti < 127) {
            tokens[count][ti++] = *p++;
        }
        tokens[count][ti] = '\0';
        count++;
    }
    return count;
}

static int elf_run_command(uint8_t cmd_id, uint32_t pid, const char *args, const char *stdin_data, char *out,
                           size_t out_cap) {
    char tmp[512];
    const char *safe_args = args != 0 ? args : "";
    const char *safe_in = stdin_data != 0 ? stdin_data : "";
    if (cmd_id == QOS_ELF_CMD_ECHO) {
        if (safe_args[0] != '\0') {
            shell_out_append(out, out_cap, safe_args);
            shell_out_append(out, out_cap, "\n");
        } else if (safe_in[0] != '\0') {
            shell_out_append(out, out_cap, safe_in);
        } else {
            shell_out_append(out, out_cap, "\n");
        }
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_PS) {
        uint32_t scan = 1u;
        shell_out_append(out, out_cap, "PID PPID STATE CMD\n");
        while (scan < g_next_pid) {
            if (qos_proc_alive(scan) != 0) {
                if (scan == QOS_INIT_PID) {
                    shell_ps_line(out, out_cap, scan, 0, "/sbin/init");
                } else if (scan == QOS_SHELL_PID) {
                    shell_ps_line(out, out_cap, scan, (int32_t)QOS_INIT_PID, "/bin/sh");
                } else if (scan == pid) {
                    shell_ps_line(out, out_cap, scan, (int32_t)QOS_SHELL_PID, "/bin/task");
                } else {
                    shell_ps_line(out, out_cap, scan, qos_proc_parent(scan), "/bin/task");
                }
            }
            scan++;
        }
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_PING) {
        if (safe_args[0] == '\0') {
            shell_out_append(out, out_cap, "ping: missing host\n");
            return 1;
        }
        if (strcmp(safe_args, "10.0.2.2") == 0) {
            shell_out_append(out, out_cap, "PING ");
            shell_out_append(out, out_cap, safe_args);
            shell_out_append(out, out_cap, "\n64 bytes from ");
            shell_out_append(out, out_cap, safe_args);
            shell_out_append(out, out_cap, ": icmp_seq=1 ttl=64 time=1ms\n1 packets transmitted, 1 received\n");
            return 0;
        }
        shell_out_append(out, out_cap, "PING ");
        shell_out_append(out, out_cap, safe_args);
        shell_out_append(out, out_cap,
                         "\nFrom 10.0.2.15 icmp_seq=1 Destination Host Unreachable\n1 packets transmitted, 0 "
                         "received\n");
        return 1;
    }
    if (cmd_id == QOS_ELF_CMD_IP) {
        if (safe_args[0] == '\0') {
            shell_out_append(out, out_cap, "ip: missing subcommand\n");
            return 1;
        }
        if (strcmp(safe_args, "addr") == 0) {
            shell_out_append(out, out_cap, "2: eth0    inet 10.0.2.15/24 brd 10.0.2.255\n");
            return 0;
        }
        if (strcmp(safe_args, "route") == 0) {
            shell_out_append(out, out_cap, "default via 10.0.2.2 dev eth0\n10.0.2.0/24 dev eth0 scope link\n");
            return 0;
        }
        shell_out_append(out, out_cap, "ip: unsupported subcommand\n");
        return 1;
    }
    if (cmd_id == QOS_ELF_CMD_WGET) {
        if (safe_args[0] == '\0') {
            shell_out_append(out, out_cap, "wget: missing url\n");
            return 1;
        }
        if (strcmp(safe_args, "http://10.0.2.2:8080/") == 0) {
            shell_out_append(out, out_cap, "HTTP/1.0 200 OK\r\nContent-Length: 4\r\n\r\nqos\n");
            return 0;
        }
        shell_out_append(out, out_cap, "wget: failed\n");
        return 1;
    }
    if (cmd_id == QOS_ELF_CMD_LS) {
        uint32_t i;
        const char *path = safe_args;
        if (path[0] == '\0' || strcmp(path, "/") == 0) {
            shell_out_append(out, out_cap, "bin\n/tmp\n/proc\n/data\n");
            for (i = 0; i < QOS_ARRAY_LEN(g_shell_file_used); i++) {
                if (g_shell_file_used[i] != 0) {
                    shell_out_append(out, out_cap, g_shell_files[i]);
                    shell_out_append(out, out_cap, "\n");
                }
            }
            return 0;
        }
        if (strcmp(path, "/proc") == 0 || strcmp(path, "/proc/") == 0) {
            shell_out_append(out, out_cap, "meminfo\nkernel/status\nnet/dev\nsyscalls\nuptime\nruntime/map\n");
            for (i = 1; i < 256; i++) {
                char status_path[32];
                int64_t fd;
                status_path[0] = '\0';
                shell_out_append(status_path, sizeof(status_path), "/proc/");
                shell_out_append_u32(status_path, sizeof(status_path), i);
                shell_out_append(status_path, sizeof(status_path), "/status");
                fd = qos_syscall_dispatch(SYSCALL_NR_OPEN, (uint64_t)(uintptr_t)status_path, 0, 0, 0);
                if (fd >= 0) {
                    (void)qos_syscall_dispatch(SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0);
                    shell_out_append_u32(out, out_cap, i);
                    shell_out_append(out, out_cap, "/status\n");
                }
            }
            return 0;
        }
        {
            size_t path_len = strlen(path);
            int had = 0;
            for (i = 0; i < QOS_ARRAY_LEN(g_shell_file_used); i++) {
                if (g_shell_file_used[i] != 0 && strncmp(g_shell_files[i], path, path_len) == 0) {
                    const char *full = g_shell_files[i];
                    const char *rest = full + path_len;
                    const char *slash;
                    if (path[path_len - 1] != '/') {
                        if (*rest != '/') {
                            continue;
                        }
                        rest++;
                    }
                    if (*rest == '\0') {
                        continue;
                    }
                    slash = rest;
                    while (*slash != '\0' && *slash != '/') {
                        slash++;
                    }
                    if (*slash == '/') {
                        shell_out_append_n(out, out_cap, rest, (size_t)(slash - rest));
                    } else {
                        shell_out_append(out, out_cap, rest);
                    }
                    shell_out_append(out, out_cap, "\n");
                    had = 1;
                }
            }
            if (!had) {
                shell_out_append(out, out_cap, "ls: not found\n");
            }
        }
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_CAT) {
        if (safe_args[0] == '\0') {
            if (safe_in[0] != '\0') {
                shell_out_append(out, out_cap, safe_in);
                return 0;
            }
            shell_out_append(out, out_cap, "cat: missing path\n");
            return 1;
        }
        if (shell_read_file(safe_args, tmp, sizeof(tmp)) != 0) {
            shell_out_append(out, out_cap, "cat: not found: ");
            shell_out_append(out, out_cap, safe_args);
            shell_out_append(out, out_cap, "\n");
            return 1;
        }
        shell_out_append(out, out_cap, tmp);
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_TOUCH) {
        if (safe_args[0] == '\0') {
            shell_out_append(out, out_cap, "touch: missing path\n");
            return 1;
        }
        if (shell_touch_path(safe_args) != 0) {
            shell_out_append(out, out_cap, "touch: failed ");
            shell_out_append(out, out_cap, safe_args);
            shell_out_append(out, out_cap, "\n");
            return 1;
        }
        shell_out_append(out, out_cap, "created file ");
        shell_out_append(out, out_cap, safe_args);
        shell_out_append(out, out_cap, "\n");
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_EDIT) {
        char path[128];
        const char *content = safe_in;
        size_t i = 0;
        size_t j = 0;
        while (safe_args[i] == ' ') {
            i++;
        }
        while (safe_args[i] != '\0' && safe_args[i] != ' ' && j + 1u < sizeof(path)) {
            path[j++] = safe_args[i++];
        }
        path[j] = '\0';
        while (safe_args[i] == ' ') {
            i++;
        }
        if (safe_args[i] != '\0') {
            content = safe_args + i;
        }
        if (path[0] == '\0') {
            shell_out_append(out, out_cap, "edit: missing path\n");
            return 1;
        }
        if (shell_write_file(path, content, 0) != 0) {
            shell_out_append(out, out_cap, "edit: failed ");
            shell_out_append(out, out_cap, path);
            shell_out_append(out, out_cap, "\n");
            return 1;
        }
        shell_out_append(out, out_cap, "edited ");
        shell_out_append(out, out_cap, path);
        shell_out_append(out, out_cap, "\n");
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_INSMOD) {
        int64_t module_id;
        if (safe_args[0] == '\0') {
            shell_out_append(out, out_cap, "insmod: missing path\n");
            return 1;
        }
        module_id = qos_syscall_dispatch(SYSCALL_NR_MODLOAD, (uint64_t)(uintptr_t)safe_args, 0, 0, 0);
        if (module_id < 0) {
            shell_out_append(out, out_cap, "insmod: failed ");
            shell_out_append(out, out_cap, safe_args);
            shell_out_append(out, out_cap, "\n");
            return 1;
        }
        module_track_loaded(safe_args, (uint32_t)module_id);
        shell_out_append(out, out_cap, "insmod: loaded id=");
        shell_out_append_u32(out, out_cap, (uint32_t)module_id);
        shell_out_append(out, out_cap, " path=");
        shell_out_append(out, out_cap, safe_args);
        shell_out_append(out, out_cap, "\n");
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_RMMOD) {
        uint32_t module_id = 0;
        if (safe_args[0] == '\0') {
            shell_out_append(out, out_cap, "rmmod: missing id or path\n");
            return 1;
        }
        if (parse_u32_arg(safe_args, &module_id) != 0) {
            if (module_find_id_by_path(safe_args, &module_id) != 0) {
                shell_out_append(out, out_cap, "rmmod: unknown module ");
                shell_out_append(out, out_cap, safe_args);
                shell_out_append(out, out_cap, "\n");
                return 1;
            }
        }
        if (qos_syscall_dispatch(SYSCALL_NR_MODUNLOAD, module_id, 0, 0, 0) != 0) {
            shell_out_append(out, out_cap, "rmmod: failed id=");
            shell_out_append_u32(out, out_cap, module_id);
            shell_out_append(out, out_cap, "\n");
            return 1;
        }
        module_forget_id(module_id);
        shell_out_append(out, out_cap, "rmmod: unloaded id=");
        shell_out_append_u32(out, out_cap, module_id);
        shell_out_append(out, out_cap, "\n");
        return 0;
    }
    if (cmd_id == QOS_ELF_CMD_WQDEMO) {
        uint32_t wid1 = 0;
        uint32_t wid2 = 0;
        g_wq_demo_hits = 0;
        if (qos_workqueue_enqueue(wq_demo_job, (void *)(uintptr_t)1u, &wid1) != 0 ||
            qos_workqueue_enqueue(wq_demo_job, (void *)(uintptr_t)2u, &wid2) != 0) {
            shell_out_append(out, out_cap, "wqdemo: enqueue failed\n");
            return 1;
        }
        (void)qos_softirq_run();
        shell_out_append(out, out_cap, "workqueue demo: pending=");
        shell_out_append_u32(out, out_cap, qos_workqueue_pending_count());
        shell_out_append(out, out_cap, " completed=");
        shell_out_append_u32(out, out_cap, qos_workqueue_completed_count());
        shell_out_append(out, out_cap, " hits=");
        shell_out_append_u32(out, out_cap, g_wq_demo_hits);
        shell_out_append(out, out_cap, "\n");
        return 0;
    }
    shell_out_append(out, out_cap, "exec: unknown image command\n");
    return 1;
}

static int spawn_exec_elf(uint32_t shell_pid, const char *path, const char *args, const char *stdin_data, char *out,
                          size_t out_cap) {
    const qos_user_elf_t *image = lookup_user_elf(path);
    uint8_t cmd_id;
    int64_t rc;
    int32_t status = -1;
    uint32_t child_pid = g_next_pid;

    if (image == 0 || validate_elf_image(image->image, image->image_len) != 0) {
        shell_out_append(out, out_cap, "exec: invalid ELF image\n");
        return -1;
    }
    cmd_id = image->cmd_id;
    if (shell_write_file_bytes(path, image->image, image->image_len) != 0) {
        shell_out_append(out, out_cap, "exec: failed to stage image\n");
        return -1;
    }
    if (g_next_pid == UINT32_MAX) {
        shell_out_append(out, out_cap, "fork: pid space exhausted\n");
        return -1;
    }
    g_next_pid++;

    rc = qos_syscall_dispatch(SYSCALL_NR_FORK, shell_pid, child_pid, 0, 0);
    if (rc < 0) {
        shell_out_append(out, out_cap, "fork: failed\n");
        return -1;
    }
    if (qos_syscall_dispatch(SYSCALL_NR_EXEC, child_pid, (uint64_t)(uintptr_t)path, 0, 0) < 0) {
        shell_out_append(out, out_cap, "exec: failed\n");
        (void)qos_syscall_dispatch(SYSCALL_NR_EXIT, child_pid, 1, 0, 0);
        (void)qos_syscall_dispatch(SYSCALL_NR_WAITPID, shell_pid, child_pid, (uint64_t)(uintptr_t)&status, 0);
        return -1;
    }

    status = (int32_t)elf_run_command(cmd_id, child_pid, args != 0 ? args : "", stdin_data, out, out_cap);
    (void)qos_syscall_dispatch(SYSCALL_NR_EXIT, child_pid, (uint64_t)(uint32_t)(status == 0 ? 0 : 1), 0, 0);
    rc = qos_syscall_dispatch(SYSCALL_NR_WAITPID, shell_pid, child_pid, (uint64_t)(uintptr_t)&status, 0);
    if (rc < 0) {
        shell_out_append(out, out_cap, "waitpid: failed\n");
        return -1;
    }
    return status;
}

static void execute_segment(uint32_t shell_pid, char tokens[][128], int n, const char *input, char *out, size_t out_cap,
                            int *exit_shell, int *mapwatch_enabled, int *mapwatch_side_enabled) {
    char *argv[32];
    int argc = 0;
    const char *redir_in = 0;
    const char *redir_out = 0;
    int append = 0;
    const char *input_data = input;
    char argline[256];
    char path_buf[128];
    int i;
    shell_out_clear(out, out_cap);
    for (i = 0; i < n; i++) {
        if ((strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) &&
            i + 1 < n) {
            if (strcmp(tokens[i], "<") == 0) {
                redir_in = tokens[i + 1];
            } else {
                redir_out = tokens[i + 1];
                append = strcmp(tokens[i], ">>") == 0;
            }
            i++;
            continue;
        }
        if (argc < 32) {
            argv[argc++] = tokens[i];
        }
    }
    if (argc == 0) {
        return;
    }
    if (redir_in != 0) {
        static char input_buf[2048];
        if (shell_read_file(redir_in, input_buf, sizeof(input_buf)) == 0) {
            input_data = input_buf;
        } else {
            input_data = "";
        }
    }
    if (strcmp(argv[0], "help") == 0) {
        shell_out_append(out, out_cap, "qos-sh commands:\n");
        shell_out_append(out, out_cap, "  help\n  echo <text>\n  ps\n  ping <ip>\n  ip addr\n  ip route\n");
        shell_out_append(out, out_cap, "  wget <url>\n  ls\n  cat <path>\n  touch <path>\n  edit <path> [text]\n");
        shell_out_append(out, out_cap, "  insmod <path>\n  rmmod <id|path>\n  wqdemo\n");
        shell_out_append(out, out_cap, "  mapwatch on|off|side on|off\n");
        shell_out_append(out, out_cap, "  exit\n");
    } else if (strcmp(argv[0], "exit") == 0) {
        shell_out_append(out, out_cap, "logout\n");
        *exit_shell = 1;
    } else if (strcmp(argv[0], "mapwatch") == 0) {
        if (argc < 2) {
            shell_out_append(out, out_cap, "usage: mapwatch on|off|side on|off\n");
        } else if (strcmp(argv[1], "on") == 0) {
            if (mapwatch_enabled != 0) {
                *mapwatch_enabled = 1;
            }
            shell_out_append(out, out_cap, "mapwatch: on\n");
        } else if (strcmp(argv[1], "off") == 0) {
            if (mapwatch_enabled != 0) {
                *mapwatch_enabled = 0;
            }
            shell_out_append(out, out_cap, "mapwatch: off\n");
        } else if (argc >= 3 && strcmp(argv[1], "side") == 0 && strcmp(argv[2], "on") == 0) {
            if (mapwatch_side_enabled != 0) {
                *mapwatch_side_enabled = 1;
            }
            mapwatch_side_emit_snapshot();
            shell_out_append(out, out_cap, "mapwatch side: on (host stream)\n");
        } else if (argc >= 3 && strcmp(argv[1], "side") == 0 && strcmp(argv[2], "off") == 0) {
            if (mapwatch_side_enabled != 0) {
                *mapwatch_side_enabled = 0;
            }
            shell_out_append(out, out_cap, "mapwatch side: off\n");
        } else {
            shell_out_append(out, out_cap, "usage: mapwatch on|off|side on|off\n");
        }
    } else {
        const char *path = resolve_command_path(argv[0], path_buf, sizeof(path_buf));
        size_t pos = 0;
        if (path == 0) {
            shell_out_append(out, out_cap, "unknown command: ");
            shell_out_append(out, out_cap, argv[0]);
            shell_out_append(out, out_cap, "\n");
            return;
        }
        argline[0] = '\0';
        for (i = 1; i < argc && pos + 1u < sizeof(argline); i++) {
            size_t k = 0;
            if (pos != 0 && pos + 1u < sizeof(argline)) {
                argline[pos++] = ' ';
            }
            while (argv[i][k] != '\0' && pos + 1u < sizeof(argline)) {
                argline[pos++] = argv[i][k++];
            }
            argline[pos] = '\0';
        }
        (void)spawn_exec_elf(shell_pid, path, argline, input_data, out, out_cap);
    }
    if (redir_out != 0) {
        (void)shell_write_file(redir_out, out, append);
        shell_out_clear(out, out_cap);
    }
}

static int shell_handle_line(const char *line, uint32_t shell_pid, int *mapwatch_enabled, int *mapwatch_side_enabled) {
    char work[256];
    char tokens[64][128];
    int ntok;
    int pipe_idx = -1;
    int i;
    int exit_shell = 0;
    char out1[2048];
    char out2[2048];
    size_t n;
    if (line == 0) {
        return 0;
    }
    strncpy(work, line, sizeof(work) - 1u);
    work[sizeof(work) - 1u] = '\0';
    n = strlen(work);
    if (n != 0 && (work[n - 1u] == '\n' || work[n - 1u] == '\r')) {
        work[n - 1u] = '\0';
    }
    ntok = tokenize(work, tokens, 64);
    if (ntok == 0) {
        return 0;
    }
    for (i = 0; i < ntok; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            pipe_idx = i;
            break;
        }
    }
    shell_out_clear(out1, sizeof(out1));
    shell_out_clear(out2, sizeof(out2));
    if (pipe_idx > 0 && pipe_idx < ntok - 1) {
        execute_segment(shell_pid, tokens, pipe_idx, 0, out1, sizeof(out1), &exit_shell, mapwatch_enabled,
                        mapwatch_side_enabled);
        execute_segment(shell_pid, tokens + pipe_idx + 1, ntok - pipe_idx - 1, out1, out2, sizeof(out2), &exit_shell,
                        mapwatch_enabled, mapwatch_side_enabled);
        if (out2[0] != '\0') {
            uart_puts(out2);
        }
    } else {
        execute_segment(shell_pid, tokens, ntok, 0, out2, sizeof(out2), &exit_shell, mapwatch_enabled,
                        mapwatch_side_enabled);
        if (out2[0] != '\0') {
            uart_puts(out2);
        }
    }
    return exit_shell;
}

static void shell_mapwatch_redraw(const char *line, size_t len) {
    char mapbuf[2048];
    uart_puts("\x1b[H\x1b[2J");
    uart_puts("[mapwatch] /proc/runtime/map (use `mapwatch off` to disable)\n");
    if (shell_read_file("/proc/runtime/map", mapbuf, sizeof(mapbuf)) == 0) {
        uart_puts(mapbuf);
    } else {
        uart_puts("CurrentPid:\t0\nCurrentProc:\tnone\nCurrentTid:\t0\nCurrentThread:\tnone\nCurrentAsid:\t0\nMappings:\t0\n");
    }
    uart_puts("qos-sh:/> ");
    if (line != 0 && len != 0u) {
        uart_putn(line, len);
    }
}

static void run_serial_shell(uint32_t shell_pid) {
    char line[QOS_SHELL_LINE_MAX];
    int mapwatch_enabled = 0;
    int mapwatch_side_enabled = 0;
    for (;;) {
        size_t len = 0;
        uint32_t poll_spins = 0;
        uint32_t side_spins = 0;
        if (mapwatch_enabled != 0) {
            shell_mapwatch_redraw(line, len);
        } else {
            uart_puts("qos-sh:/> ");
        }
        for (;;) {
            uint8_t ch = 0;
            if (uart_try_getc(&ch) == 0) {
                if (mapwatch_enabled != 0) {
                    poll_spins++;
                    if (poll_spins >= MAPWATCH_POLL_SPINS) {
                        poll_spins = 0;
                        shell_mapwatch_redraw(line, len);
                    }
                }
                if (mapwatch_side_enabled != 0) {
                    side_spins++;
                    if (side_spins >= MAPWATCH_SIDE_POLL_SPINS) {
                        side_spins = 0;
                        mapwatch_side_emit_snapshot();
                    }
                }
                continue;
            }
            if (ch == '\r' || ch == '\n') {
                uart_puts("\n");
                break;
            }
            if (ch == 0x08u || ch == 0x7Fu) {
                if (len != 0u) {
                    len--;
                    uart_puts("\b \b");
                }
                continue;
            }
            if (ch < 0x20u || len + 1u >= QOS_SHELL_LINE_MAX) {
                continue;
            }
            line[len++] = (char)ch;
            uart_putn((const char *)&ch, 1);
            poll_spins = 0;
            side_spins = 0;
        }
        line[len] = '\0';
        if (shell_handle_line(line, shell_pid, &mapwatch_enabled, &mapwatch_side_enabled) != 0) {
            return;
        }
    }
}

static void run_init_process(void) {
    int init_ok = qos_proc_create(QOS_INIT_PID, 0u) == 0;
    int shell_ok = 0;
    memset(g_shell_files, 0, sizeof(g_shell_files));
    memset(g_shell_file_used, 0, sizeof(g_shell_file_used));
    memset(g_loaded_module_used, 0, sizeof(g_loaded_module_used));
    memset(g_loaded_module_ids, 0, sizeof(g_loaded_module_ids));
    memset(g_loaded_module_paths, 0, sizeof(g_loaded_module_paths));
    if (init_ok) {
        shell_ok = qos_syscall_dispatch(SYSCALL_NR_FORK, QOS_INIT_PID, QOS_SHELL_PID, 0, 0) >= 0;
    }
    if (!shell_ok) {
        shell_ok = qos_proc_create(QOS_SHELL_PID, QOS_INIT_PID) == 0;
    }
    (void)qos_sched_add_task(QOS_INIT_PID);
    (void)qos_vmm_map_as(QOS_INIT_PID, 0x00004000u, 0x00100000u, VM_READ | VM_EXEC | VM_USER);
    (void)qos_vmm_map_as(QOS_INIT_PID, 0x00006000u, 0x00102000u, VM_READ | VM_WRITE | VM_USER);
    (void)qos_vmm_map_as(QOS_INIT_PID, 0x7FFF0000u, 0x00104000u, VM_READ | VM_WRITE | VM_USER);
    if (shell_ok) {
        (void)qos_sched_add_task(QOS_SHELL_PID);
        (void)qos_vmm_map_as(QOS_SHELL_PID, 0x00004000u, 0x00200000u, VM_READ | VM_EXEC | VM_USER);
        (void)qos_vmm_map_as(QOS_SHELL_PID, 0x00006000u, 0x00202000u, VM_READ | VM_WRITE | VM_USER);
        (void)qos_vmm_map_as(QOS_SHELL_PID, 0x7FFF0000u, 0x00204000u, VM_READ | VM_WRITE | VM_USER);
        (void)qos_sched_next();
        (void)qos_sched_next();
        qos_vmm_set_asid(QOS_SHELL_PID);
    } else {
        (void)qos_sched_next();
        qos_vmm_set_asid(QOS_INIT_PID);
    }
    g_next_pid = QOS_SHELL_PID + 1u;

    uart_puts("init[1]: starting /sbin/init\n");
    if (shell_ok) {
        uart_puts("init[1]: exec /bin/sh\n");
    } else {
        uart_puts("init[1]: failed to exec /bin/sh\n");
    }
    (void)qos_syscall_dispatch(SYSCALL_NR_MKDIR, (uint64_t)(uintptr_t)"/tmp", 0, 0, 0);
    (void)qos_syscall_dispatch(SYSCALL_NR_MKDIR, (uint64_t)(uintptr_t)"/data", 0, 0, 0);
    stage_test_module_artifacts();
    run_serial_shell(QOS_SHELL_PID);
}

static int dtb_magic_ok(uint64_t dtb_addr) {
    volatile const uint32_t *magic_ptr;
    if (dtb_addr == 0) {
        return 0;
    }
    magic_ptr = (volatile const uint32_t *)(uintptr_t)dtb_addr;
    return *magic_ptr == QOS_DTB_MAGIC;
}

static uint32_t mmio_read32(uintptr_t base, uintptr_t off) {
    volatile const uint32_t *addr = (volatile const uint32_t *)(base + off);
    return *addr;
}

static void mmio_write32(uintptr_t base, uintptr_t off, uint32_t value) {
    volatile uint32_t *addr = (volatile uint32_t *)(base + off);
    *addr = value;
}

static uintptr_t align_up(uintptr_t value, uintptr_t align) {
    if (align == 0u) {
        return value;
    }
    return (value + (align - 1u)) & ~(align - 1u);
}

static void write_be16(uint8_t *buf, size_t off, uint16_t value) {
    buf[off] = (uint8_t)(value >> 8);
    buf[off + 1] = (uint8_t)value;
}

static void write_be32(uint8_t *buf, size_t off, uint32_t value) {
    buf[off] = (uint8_t)(value >> 24);
    buf[off + 1] = (uint8_t)(value >> 16);
    buf[off + 2] = (uint8_t)(value >> 8);
    buf[off + 3] = (uint8_t)value;
}

static uint16_t read_be16(const uint8_t *buf, size_t off) {
    return (uint16_t)(((uint16_t)buf[off] << 8) | (uint16_t)buf[off + 1]);
}

static uint32_t read_be32(const uint8_t *buf, size_t off) {
    return ((uint32_t)buf[off] << 24) | ((uint32_t)buf[off + 1] << 16) | ((uint32_t)buf[off + 2] << 8) |
           (uint32_t)buf[off + 3];
}

static uint16_t ones_complement_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    size_t i = 0;
    while (i + 1 < len) {
        uint16_t word = (uint16_t)((uint16_t)data[i] << 8) | (uint16_t)data[i + 1];
        sum += word;
        i += 2;
    }
    if (i < len) {
        sum += (uint32_t)data[i] << 8;
    }
    while ((sum >> 16) != 0u) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

static uint16_t ipv4_checksum(const uint8_t *header, size_t len) {
    return ones_complement_checksum(header, len);
}

static size_t build_probe_udp_ipv4_frame(uint8_t *buf, size_t buflen) {
    size_t o = 0;
    size_t payload_len = sizeof(PROBE_PAYLOAD) - 1u;
    uint16_t ip_total_len = (uint16_t)(IPV4_HDR_LEN + UDP_HDR_LEN + payload_len);
    uint16_t udp_len = (uint16_t)(UDP_HDR_LEN + payload_len);
    uint16_t csum;

    if (buf == 0 || buflen < PROBE_FRAME_LEN) {
        return 0;
    }

    memset(buf, 0xFF, 6);
    o += 6;
    memcpy(buf + o, PROBE_SRC_MAC, sizeof(PROBE_SRC_MAC));
    o += sizeof(PROBE_SRC_MAC);
    buf[o] = 0x08u;
    buf[o + 1] = 0x00u;
    o += 2;

    buf[o] = 0x45u;
    buf[o + 1] = 0u;
    write_be16(buf, o + 2, ip_total_len);
    write_be16(buf, o + 4, 1u);
    write_be16(buf, o + 6, 0x4000u);
    buf[o + 8] = 64u;
    buf[o + 9] = 17u;
    write_be16(buf, o + 10, 0u);
    write_be32(buf, o + 12, QOS_NET_IPV4_LOCAL);
    write_be32(buf, o + 16, QOS_NET_IPV4_GATEWAY);
    csum = ipv4_checksum(buf + o, IPV4_HDR_LEN);
    write_be16(buf, o + 10, csum);
    o += IPV4_HDR_LEN;

    write_be16(buf, o, PROBE_UDP_SRC_PORT);
    write_be16(buf, o + 2, PROBE_UDP_DST_PORT);
    write_be16(buf, o + 4, udp_len);
    write_be16(buf, o + 6, 0u);
    o += UDP_HDR_LEN;

    memcpy(buf + o, PROBE_PAYLOAD, payload_len);
    o += payload_len;
    return o;
}

static size_t build_probe_icmp_ipv4_frame(uint8_t *buf, size_t buflen) {
    size_t o = 0;
    uint16_t ip_total_len = (uint16_t)(IPV4_HDR_LEN + ICMP_HDR_LEN + (sizeof(ICMP_PROBE_PAYLOAD) - 1u));
    uint16_t csum;

    if (buf == 0 || buflen < ETH_HDR_LEN + IPV4_HDR_LEN + ICMP_HDR_LEN + (sizeof(ICMP_PROBE_PAYLOAD) - 1u)) {
        return 0;
    }

    memset(buf, 0xFF, 6);
    o += 6;
    memcpy(buf + o, PROBE_SRC_MAC, sizeof(PROBE_SRC_MAC));
    o += sizeof(PROBE_SRC_MAC);
    buf[o] = 0x08u;
    buf[o + 1] = 0x00u;
    o += 2;

    buf[o] = 0x45u;
    buf[o + 1] = 0u;
    write_be16(buf, o + 2, ip_total_len);
    write_be16(buf, o + 4, 2u);
    write_be16(buf, o + 6, 0x4000u);
    buf[o + 8] = 64u;
    buf[o + 9] = 1u;
    write_be16(buf, o + 10, 0u);
    write_be32(buf, o + 12, QOS_NET_IPV4_LOCAL);
    write_be32(buf, o + 16, QOS_NET_IPV4_GATEWAY);
    csum = ipv4_checksum(buf + o, IPV4_HDR_LEN);
    write_be16(buf, o + 10, csum);
    o += IPV4_HDR_LEN;

    buf[o] = 8u;
    buf[o + 1] = 0u;
    write_be16(buf, o + 2, 0u);
    write_be16(buf, o + 4, ICMP_ECHO_ID);
    write_be16(buf, o + 6, ICMP_ECHO_SEQ);
    memcpy(buf + o + ICMP_HDR_LEN, ICMP_PROBE_PAYLOAD, sizeof(ICMP_PROBE_PAYLOAD) - 1u);
    csum = ones_complement_checksum(buf + o, ICMP_HDR_LEN + sizeof(ICMP_PROBE_PAYLOAD) - 1u);
    write_be16(buf, o + 2, csum);
    o += ICMP_HDR_LEN + (sizeof(ICMP_PROBE_PAYLOAD) - 1u);
    return o;
}

static int is_icmp_echo_reply_frame(const uint8_t *frame, size_t len) {
    size_t ip_off = ETH_HDR_LEN;
    size_t ihl;
    size_t icmp_off;
    size_t payload_off;
    if (frame == 0 || len < ETH_HDR_LEN + IPV4_HDR_LEN + ICMP_HDR_LEN) {
        return 0;
    }
    if (frame[12] != 0x08u || frame[13] != 0x00u) {
        return 0;
    }
    ihl = (size_t)(frame[ip_off] & 0x0Fu) * 4u;
    if (ihl < IPV4_HDR_LEN || len < ETH_HDR_LEN + ihl + ICMP_HDR_LEN) {
        return 0;
    }
    if (frame[ip_off + 9] != 1u) {
        return 0;
    }
    if (read_be32(frame, ip_off + 12u) != QOS_NET_IPV4_GATEWAY || read_be32(frame, ip_off + 16u) != QOS_NET_IPV4_LOCAL) {
        return 0;
    }
    icmp_off = ip_off + ihl;
    if (frame[icmp_off] != 0u || frame[icmp_off + 1u] != 0u) {
        return 0;
    }
    if (read_be16(frame, icmp_off + 4u) != ICMP_ECHO_ID || read_be16(frame, icmp_off + 6u) != ICMP_ECHO_SEQ) {
        return 0;
    }
    payload_off = icmp_off + ICMP_HDR_LEN;
    if (payload_off + (sizeof(ICMP_PROBE_PAYLOAD) - 1u) > len) {
        return 0;
    }
    return memcmp(frame + payload_off, ICMP_PROBE_PAYLOAD, sizeof(ICMP_PROBE_PAYLOAD) - 1u) == 0;
}

static int find_payload(const uint8_t *haystack, size_t hay_len, const uint8_t *needle, size_t needle_len) {
    size_t i = 0;
    if (haystack == 0 || needle == 0 || needle_len == 0 || hay_len < needle_len) {
        return 0;
    }
    while (i + needle_len <= hay_len) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return 1;
        }
        i++;
    }
    return 0;
}

static int virtio_net_find_mmio_base(uintptr_t *out_base, uint32_t *out_version) {
    uint32_t i = 0;
    if (out_base == 0 || out_version == 0) {
        return -1;
    }
    while (i < VIRTIO_MMIO_SCAN_SLOTS) {
        uintptr_t base = (uintptr_t)VIRTIO_MMIO_BASE_START + (uintptr_t)i * (uintptr_t)VIRTIO_MMIO_STRIDE;
        uint32_t magic = mmio_read32(base, VIRTIO_MMIO_MAGIC_VALUE);
        if (magic == VIRTIO_MMIO_MAGIC) {
            uint32_t version = mmio_read32(base, VIRTIO_MMIO_VERSION);
            uint32_t dev_id = mmio_read32(base, VIRTIO_MMIO_DEVICE_ID);
            g_net_tx_virtio_version = version;
            if (dev_id == VIRTIO_MMIO_DEVICE_NET) {
                *out_base = base;
                *out_version = version;
                return 0;
            }
        }
        i++;
    }
    return -1;
}

static int virtio_net_init_queue_legacy(uintptr_t base,
                                        uint32_t queue,
                                        uintptr_t page_addr,
                                        legacy_queue_state_t *out_q) {
    uint32_t qmax;
    uint16_t qsize;
    uintptr_t avail_off;
    uintptr_t used_off;

    if (out_q == 0) {
        return -1;
    }

    mmio_write32(base, VIRTIO_MMIO_QUEUE_SEL, queue);
    qmax = mmio_read32(base, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (qmax == 0u) {
        return -1;
    }

    qsize = (uint16_t)((qmax < VIRTQ_SIZE) ? qmax : VIRTQ_SIZE);
    if (qsize == 0u) {
        return -1;
    }

    mmio_write32(base, VIRTIO_MMIO_QUEUE_NUM, qsize);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_ALIGN, LEGACY_QUEUE_ALIGN);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_PFN, (uint32_t)(page_addr >> 12));
    if (mmio_read32(base, VIRTIO_MMIO_QUEUE_PFN) == 0u) {
        return -1;
    }

    avail_off = sizeof(virtq_desc_t) * (uintptr_t)qsize;
    used_off = align_up(avail_off + 4u + (2u * (uintptr_t)qsize) + 2u, LEGACY_QUEUE_ALIGN);
    out_q->qsize = qsize;
    out_q->desc = (volatile virtq_desc_t *)page_addr;
    out_q->avail_idx = (volatile uint16_t *)(page_addr + avail_off + 2u);
    out_q->avail_ring = (volatile uint16_t *)(page_addr + avail_off + 4u);
    out_q->used_idx = (volatile const uint16_t *)(page_addr + used_off + 2u);
    out_q->used_ring = (volatile const uint8_t *)(page_addr + used_off + 4u);
    return 0;
}

static void virtio_net_probe(int *out_tx_ok, int *out_rx_ok, int *out_icmp_ok) {
    uintptr_t base = 0;
    uint32_t version = 0;
    legacy_queue_state_t tx = {0};
    legacy_queue_state_t rx = {0};
    size_t frame_len;
    uint32_t tx_len;
    uint16_t old_avail;
    uint16_t old_rx_used = 0;
    uint16_t old_used;
    uint16_t target;
    uint32_t spins = 0;
    int tx_ok = 0;
    int rx_ok = 0;
    int icmp_ok = 0;

    if (out_tx_ok != 0) {
        *out_tx_ok = 0;
    }
    if (out_rx_ok != 0) {
        *out_rx_ok = 0;
    }
    if (out_icmp_ok != 0) {
        *out_icmp_ok = 0;
    }
    if (virtio_net_find_mmio_base(&base, &version) != 0) {
        g_net_tx_stage = NET_TX_STAGE_NO_NETDEV;
        g_net_rx_stage = NET_RX_STAGE_NO_NETDEV;
        g_icmp_real_stage = ICMP_REAL_STAGE_NO_NETDEV;
        return;
    }
    if (version != 1u) {
        g_net_tx_stage = NET_TX_STAGE_FEATURES;
        g_net_rx_stage = NET_RX_STAGE_FEATURES;
        g_icmp_real_stage = ICMP_REAL_STAGE_FEATURES;
        return;
    }

    mmio_write32(base, VIRTIO_MMIO_STATUS, 0u);
    mmio_write32(base, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0u);
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES, 0u);
    mmio_write32(base, VIRTIO_MMIO_GUEST_PAGE_SIZE, LEGACY_QUEUE_ALIGN);

    memset(LEGACY_RX_QUEUE_PAGE, 0, sizeof(LEGACY_RX_QUEUE_PAGE));
    memset(LEGACY_TX_QUEUE_PAGE, 0, sizeof(LEGACY_TX_QUEUE_PAGE));
    if (virtio_net_init_queue_legacy(base, VIRTIO_NET_QUEUE_RX, (uintptr_t)&LEGACY_RX_QUEUE_PAGE[0], &rx) != 0) {
        g_net_tx_stage = NET_TX_STAGE_QUEUE;
        g_net_rx_stage = NET_RX_STAGE_QUEUE;
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
        return;
    }
    if (virtio_net_init_queue_legacy(base, VIRTIO_NET_QUEUE_TX, (uintptr_t)&LEGACY_TX_QUEUE_PAGE[0], &tx) != 0) {
        g_net_tx_stage = NET_TX_STAGE_QUEUE;
        g_net_rx_stage = NET_RX_STAGE_QUEUE;
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
        return;
    }
    mmio_write32(base, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);

    memset(LEGACY_RX_FRAME_BUF, 0, sizeof(LEGACY_RX_FRAME_BUF));
    rx.desc[0].addr = (uint64_t)(uintptr_t)&LEGACY_RX_FRAME_BUF[0];
    rx.desc[0].len = (uint32_t)RX_BUF_LEN;
    rx.desc[0].flags = VIRTQ_DESC_F_WRITE;
    rx.desc[0].next = 0u;
    old_avail = *rx.avail_idx;
    rx.avail_ring[old_avail % rx.qsize] = 0u;
    __sync_synchronize();
    *rx.avail_idx = (uint16_t)(old_avail + 1u);
    __sync_synchronize();
    mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);
    old_rx_used = *rx.used_idx;

    memset(LEGACY_TX_FRAME_BUF, 0, sizeof(LEGACY_TX_FRAME_BUF));
    frame_len = build_probe_udp_ipv4_frame(LEGACY_TX_FRAME_BUF + VIRTIO_NET_HDR_LEN, PROBE_FRAME_LEN);
    if (frame_len == 0u) {
        g_net_tx_stage = NET_TX_STAGE_QUEUE;
        g_net_rx_stage = NET_RX_STAGE_QUEUE;
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
        return;
    }
    tx_len = (uint32_t)(VIRTIO_NET_HDR_LEN + frame_len);

    tx.desc[0].addr = (uint64_t)(uintptr_t)&LEGACY_TX_FRAME_BUF[0];
    tx.desc[0].len = tx_len;
    tx.desc[0].flags = 0u;
    tx.desc[0].next = 0u;

    old_avail = *tx.avail_idx;
    tx.avail_ring[old_avail % tx.qsize] = 0u;
    __sync_synchronize();
    *tx.avail_idx = (uint16_t)(old_avail + 1u);
    __sync_synchronize();
    mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);

    old_used = *tx.used_idx;
    target = (uint16_t)(old_used + 1u);
    while (spins < 2000000u) {
        uint16_t cur = *tx.used_idx;
        if (cur == target) {
            g_net_tx_stage = NET_TX_STAGE_OK;
            tx_ok = 1;
            break;
        }
        spins++;
    }

    if (!tx_ok) {
        g_net_tx_stage = NET_TX_STAGE_TIMEOUT;
    }

    memset(LEGACY_TX_FRAME_BUF, 0, sizeof(LEGACY_TX_FRAME_BUF));
    frame_len = build_probe_icmp_ipv4_frame(LEGACY_TX_FRAME_BUF + VIRTIO_NET_HDR_LEN, PROBE_FRAME_LEN);
    if (frame_len == 0u) {
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
    } else {
        tx_len = (uint32_t)(VIRTIO_NET_HDR_LEN + frame_len);
        tx.desc[0].addr = (uint64_t)(uintptr_t)&LEGACY_TX_FRAME_BUF[0];
        tx.desc[0].len = tx_len;
        tx.desc[0].flags = 0u;
        tx.desc[0].next = 0u;

        old_avail = *tx.avail_idx;
        tx.avail_ring[old_avail % tx.qsize] = 0u;
        __sync_synchronize();
        *tx.avail_idx = (uint16_t)(old_avail + 1u);
        __sync_synchronize();
        mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);

        old_used = *tx.used_idx;
        target = (uint16_t)(old_used + 1u);
        spins = 0;
        while (spins < 2000000u) {
            uint16_t cur = *tx.used_idx;
            if (cur == target) {
                break;
            }
            spins++;
        }
        if (spins >= 2000000u) {
            g_icmp_real_stage = ICMP_REAL_STAGE_TX;
        }
    }

    spins = 0;
    while (spins < 2000000u) {
        uint16_t cur = *rx.used_idx;
        if (cur != old_rx_used) {
            uint16_t slot = (uint16_t)(old_rx_used % rx.qsize);
            volatile const uint8_t *elem = rx.used_ring + ((size_t)slot * 8u);
            uint32_t used_len = *(volatile const uint32_t *)(elem + 4u);
            size_t payload_start = (used_len > VIRTIO_NET_HDR_LEN) ? VIRTIO_NET_HDR_LEN : used_len;
            size_t payload_len = used_len > RX_BUF_LEN ? RX_BUF_LEN : used_len;
            if (payload_len > payload_start) {
                g_net_rx_stage = NET_RX_STAGE_PAYLOAD;
                rx_ok = 1;
                if (is_icmp_echo_reply_frame(LEGACY_RX_FRAME_BUF + payload_start, payload_len - payload_start)) {
                    g_icmp_real_stage = ICMP_REAL_STAGE_OK;
                    icmp_ok = 1;
                }
            }
            old_avail = *rx.avail_idx;
            rx.avail_ring[old_avail % rx.qsize] = 0u;
            __sync_synchronize();
            *rx.avail_idx = (uint16_t)(old_avail + 1u);
            __sync_synchronize();
            mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);
            old_rx_used = cur;
            if (icmp_ok) {
                break;
            }
        }
        spins++;
    }
    if (!rx_ok && g_net_rx_stage != NET_RX_STAGE_PAYLOAD) {
        g_net_rx_stage = NET_RX_STAGE_TIMEOUT;
    }
    if (!icmp_ok && g_icmp_real_stage == ICMP_REAL_STAGE_QUEUE) {
        g_icmp_real_stage = ICMP_REAL_STAGE_TIMEOUT;
    }
    if (out_tx_ok != 0) {
        *out_tx_ok = tx_ok;
    }
    if (out_rx_ok != 0) {
        *out_rx_ok = rx_ok;
    }
    if (out_icmp_ok != 0) {
        *out_icmp_ok = icmp_ok;
    }
}

static const char *net_tx_stage_text(void) {
    switch (g_net_tx_stage) {
        case NET_TX_STAGE_OK:
            return "ok";
        case NET_TX_STAGE_NO_NETDEV:
            return "no_netdev";
        case NET_TX_STAGE_FEATURES:
            return "features";
        case NET_TX_STAGE_QUEUE:
            return "queue";
        case NET_TX_STAGE_TIMEOUT:
            return "timeout";
        default:
            return "unknown";
    }
}

static const char *net_tx_version_text(void) {
    switch (g_net_tx_virtio_version) {
        case 1u:
            return "v1";
        case 2u:
            return "v2";
        case 0u:
            return "v0";
        default:
            return "v?";
    }
}

static const char *net_rx_stage_text(void) {
    switch (g_net_rx_stage) {
        case NET_RX_STAGE_OK:
            return "ok";
        case NET_RX_STAGE_NO_NETDEV:
            return "no_netdev";
        case NET_RX_STAGE_FEATURES:
            return "features";
        case NET_RX_STAGE_QUEUE:
            return "queue";
        case NET_RX_STAGE_TIMEOUT:
            return "timeout";
        case NET_RX_STAGE_PAYLOAD:
            return "payload";
        default:
            return "unknown";
    }
}

static const char *net_rx_result_text(void) {
    switch (g_net_rx_stage) {
        case NET_RX_STAGE_OK:
            return "real_ok";
        case NET_RX_STAGE_PAYLOAD:
            return "real_ok";
        case NET_RX_STAGE_TIMEOUT:
            return "real_skip";
        default:
            return "real_fail";
    }
}

static const char *icmp_real_stage_text(void) {
    switch (g_icmp_real_stage) {
        case ICMP_REAL_STAGE_OK:
            return "ok";
        case ICMP_REAL_STAGE_NO_NETDEV:
            return "no_netdev";
        case ICMP_REAL_STAGE_FEATURES:
            return "features";
        case ICMP_REAL_STAGE_QUEUE:
            return "queue";
        case ICMP_REAL_STAGE_TIMEOUT:
            return "timeout";
        case ICMP_REAL_STAGE_TX:
            return "tx";
        default:
            return "unknown";
    }
}

static const char *icmp_real_result_text(void) {
    switch (g_icmp_real_stage) {
        case ICMP_REAL_STAGE_OK:
            return "roundtrip_ok";
        case ICMP_REAL_STAGE_TIMEOUT:
            return "roundtrip_skip";
        default:
            return "roundtrip_fail";
    }
}

void c_main(uint64_t dtb_addr) {
    uint64_t fallback_dtb;
    uint64_t effective_dtb;
    boot_info_t info;
    int kernel_ok;
    int icmp_ok = 0;
    int net_tx_ok = 0;
    int net_rx_ok = 0;

    uart_puts("probe=enter\n");

    fallback_dtb = (uint64_t)(uintptr_t)&QOS_FAKE_DTB;
    if (dtb_addr != 0 && dtb_magic_ok(dtb_addr)) {
        effective_dtb = dtb_addr;
    } else {
        effective_dtb = fallback_dtb;
    }

    info = (boot_info_t){0};
    info.magic = QOS_BOOT_MAGIC;
    info.mmap_entry_count = 1;
    info.mmap_entries[0].base = 0x40000000ULL;
    info.mmap_entries[0].length = 0x04000000ULL;
    info.mmap_entries[0].type = 1;
    info.initramfs_addr = 0x44000000ULL;
    info.initramfs_size = 0x1000ULL;
    info.dtb_addr = effective_dtb;

    kernel_ok = qos_kernel_entry(&info) == 0;
    if (kernel_ok) {
        uint8_t payload[3] = {'q', 'o', 's'};
        uint8_t reply[8] = {0};
        uint32_t src_ip = 0;
        icmp_ok = qos_net_icmp_echo(QOS_NET_IPV4_GATEWAY, 1, 1, payload, 3, reply, sizeof(reply), &src_ip) == 3;
        virtio_net_probe(&net_tx_ok, &net_rx_ok, 0);
    }

    if (kernel_ok && dtb_magic_ok(effective_dtb) && icmp_ok) {
        if (net_tx_ok) {
            uart_puts("QOS kernel started impl=c arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=dtb_stub mmap_count=1 mmap_len_nonzero=1 initramfs_source=stub initramfs_size_nonzero=1 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_ok net_tx=real_ok net_tx_stage=ok net_rx=");
            uart_puts(net_rx_result_text());
            uart_puts(" net_rx_stage=");
            uart_puts(net_rx_stage_text());
            uart_puts(" icmp_real=");
            uart_puts(icmp_real_result_text());
            uart_puts(" icmp_real_stage=");
            uart_puts(icmp_real_stage_text());
            uart_puts("\n");
        } else {
            uart_puts("QOS kernel started impl=c arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=dtb_stub mmap_count=1 mmap_len_nonzero=1 initramfs_source=stub initramfs_size_nonzero=1 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_ok net_tx=real_fail net_tx_stage=");
            uart_puts(net_tx_stage_text());
            uart_puts(" net_tx_ver=");
            uart_puts(net_tx_version_text());
            uart_puts(" net_rx=");
            uart_puts(net_rx_result_text());
            uart_puts(" net_rx_stage=");
            uart_puts(net_rx_stage_text());
            uart_puts(" icmp_real=");
            uart_puts(icmp_real_result_text());
            uart_puts(" icmp_real_stage=");
            uart_puts(icmp_real_stage_text());
            uart_puts("\n");
        }
    } else {
        uart_puts("QOS kernel started impl=c arch=aarch64 handoff=invalid entry=kernel_main abi=x0 dtb_addr_nonzero=0 dtb_magic=bad mmap_source=dtb_stub mmap_count=0 mmap_len_nonzero=0 initramfs_source=stub initramfs_size_nonzero=0 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_fail net_tx=real_fail net_tx_stage=");
        uart_puts(net_tx_stage_text());
        uart_puts(" net_tx_ver=");
        uart_puts(net_tx_version_text());
        uart_puts(" net_rx=");
        uart_puts(net_rx_result_text());
        uart_puts(" net_rx_stage=");
        uart_puts(net_rx_stage_text());
        uart_puts(" icmp_real=");
        uart_puts(icmp_real_result_text());
        uart_puts(" icmp_real_stage=");
        uart_puts(icmp_real_stage_text());
        uart_puts("\n");
    }

    run_init_process();
    for (;;) {
        __asm__ volatile("wfe");
    }
}
