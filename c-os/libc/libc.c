#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__GNUC__)
static __thread int g_qos_errno = 0;
#else
static int g_qos_errno = 0;
#endif

#if defined(__GNUC__)
extern int64_t qos_syscall_dispatch(uint32_t nr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
    __attribute__((weak));
extern int qos_proc_signal_altstack_set(uint32_t pid, uint64_t sp, uint64_t size, uint32_t flags)
    __attribute__((weak));
extern int qos_proc_signal_altstack_get(uint32_t pid, void *out_altstack) __attribute__((weak));
#else
extern int64_t qos_syscall_dispatch(uint32_t nr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3);
extern int qos_proc_signal_altstack_set(uint32_t pid, uint64_t sp, uint64_t size, uint32_t flags);
extern int qos_proc_signal_altstack_get(uint32_t pid, void *out_altstack);
#endif

#define QOS_LIBC_SYSCALL_NR_SOCKET 14u
#define QOS_LIBC_SYSCALL_NR_BIND 15u
#define QOS_LIBC_SYSCALL_NR_LISTEN 16u
#define QOS_LIBC_SYSCALL_NR_ACCEPT 17u
#define QOS_LIBC_SYSCALL_NR_CONNECT 18u
#define QOS_LIBC_SYSCALL_NR_SEND 19u
#define QOS_LIBC_SYSCALL_NR_RECV 20u
#define QOS_LIBC_SYSCALL_NR_CLOSE 8u
#define QOS_LIBC_SYSCALL_NR_KILL 29u
#define QOS_LIBC_SYSCALL_NR_GETPID 3u
#define QOS_LIBC_SYSCALL_NR_SIGACTION 30u
#define QOS_LIBC_SYSCALL_NR_SIGPROCMASK 31u
#define QOS_LIBC_SYSCALL_NR_SIGPENDING 33u
#define QOS_LIBC_SYSCALL_NR_SIGSUSPEND 34u
#define QOS_LIBC_SYSCALL_NR_SIGALTSTACK 35u
#define QOS_LIBC_SYSCALL_NR_FORK 0u
#define QOS_LIBC_SYSCALL_NR_EXEC 1u
#define QOS_LIBC_SYSCALL_NR_EXIT 2u
#define QOS_LIBC_SYSCALL_NR_WAITPID 4u
#define QOS_LIBC_SYSCALL_NR_OPEN 5u
#define QOS_LIBC_SYSCALL_NR_READ 6u
#define QOS_LIBC_SYSCALL_NR_WRITE 7u
#define QOS_LIBC_SYSCALL_NR_LSEEK 25u
#define QOS_LIBC_SYSCALL_NR_MKDIR 22u
#define QOS_LIBC_SYSCALL_NR_CHDIR 27u
#define QOS_LIBC_SYSCALL_NR_GETCWD 28u

#define QOS_LIBC_HEAP_BYTES (1u << 20)
#define QOS_LIBC_HEAP_ALIGN 16u
#define QOS_LIBC_FILE_MAX 64u
#define QOS_LIBC_FILE_MODE_READ 0x1u
#define QOS_LIBC_FILE_MODE_WRITE 0x2u
#define QOS_LIBC_FILE_MODE_APPEND 0x4u
#define QOS_LIBC_BACKEND_HOST 0u
#define QOS_LIBC_BACKEND_KERNEL 1u

typedef struct qos_libc_alloc_block {
    size_t size;
    uint32_t free;
    struct qos_libc_alloc_block *next;
} qos_libc_alloc_block_t;

typedef struct {
    uint8_t used;
    uint8_t backend;
    uint16_t _pad;
    int32_t fd;
    uint32_t mode;
} qos_libc_file_t;

typedef struct {
    uint64_t sp;
    uint64_t size;
    uint32_t flags;
    uint32_t _pad;
} qos_libc_kernel_sigaltstack_t;

int qos_snprintf(char *dst, size_t len, const char *fmt, ...);
int qos_vsnprintf(char *dst, size_t len, const char *fmt, va_list ap);
char *qos_strchr(const char *s, int c);
int qos_raise(int signum);

extern char **environ;

static uint8_t g_qos_heap[QOS_LIBC_HEAP_BYTES];
static qos_libc_alloc_block_t *g_qos_heap_head = 0;
static qos_libc_file_t g_qos_files[QOS_LIBC_FILE_MAX];
static uint8_t g_qos_file_init = 0;

static int qos_libc_have_kernel_syscall(void) {
    return qos_syscall_dispatch != 0;
}

static int64_t qos_libc_syscall4(uint32_t nr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, int *out_used) {
    if (!qos_libc_have_kernel_syscall()) {
        if (out_used != NULL) {
            *out_used = 0;
        }
        return -1;
    }
    if (out_used != NULL) {
        *out_used = 1;
    }
    return qos_syscall_dispatch(nr, a0, a1, a2, a3);
}

static uint32_t qos_libc_sigset_load(const sigset_t *set) {
    uint32_t mask = 0;
    if (set == NULL) {
        return 0;
    }
    memcpy(&mask, set, sizeof(mask));
    return mask;
}

static void qos_libc_sigset_store(sigset_t *set, uint32_t mask) {
    if (set == NULL) {
        return;
    }
    memset(set, 0, sizeof(*set));
    memcpy(set, &mask, sizeof(mask));
}

static uint32_t qos_libc_resolve_pid(void) {
    static __thread uint32_t g_cached_pid = 0;
    int used = 0;
    int64_t rc;
    uint32_t candidate;

    if (!qos_libc_have_kernel_syscall()) {
        return 0;
    }

    if (g_cached_pid != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_GETPID, g_cached_pid, 0, 0, 0, &used);
        if (used != 0 && rc == (int64_t)g_cached_pid) {
            return g_cached_pid;
        }
    }

    for (candidate = 1; candidate <= 4096u; candidate++) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_GETPID, candidate, 0, 0, 0, &used);
        if (used != 0 && rc == (int64_t)candidate) {
            g_cached_pid = candidate;
            return candidate;
        }
    }

    return 0;
}

static size_t qos_libc_align_up(size_t value, size_t align) {
    return (value + (align - 1u)) & ~(align - 1u);
}

static int64_t qos_libc_host_syscall6(long nr, long a0, long a1, long a2, long a3, long a4, long a5) {
    return (int64_t)syscall(nr, a0, a1, a2, a3, a4, a5);
}

static void qos_libc_heap_init(void) {
    if (g_qos_heap_head != 0) {
        return;
    }
    g_qos_heap_head = (qos_libc_alloc_block_t *)(void *)g_qos_heap;
    g_qos_heap_head->size = QOS_LIBC_HEAP_BYTES - sizeof(qos_libc_alloc_block_t);
    g_qos_heap_head->free = 1u;
    g_qos_heap_head->next = 0;
}

static void qos_libc_heap_split(qos_libc_alloc_block_t *blk, size_t need) {
    qos_libc_alloc_block_t *next_blk;
    if (blk == 0 || blk->size <= need + sizeof(qos_libc_alloc_block_t) + QOS_LIBC_HEAP_ALIGN) {
        return;
    }
    next_blk = (qos_libc_alloc_block_t *)(void *)((uint8_t *)(void *)(blk + 1) + need);
    next_blk->size = blk->size - need - sizeof(qos_libc_alloc_block_t);
    next_blk->free = 1u;
    next_blk->next = blk->next;
    blk->size = need;
    blk->next = next_blk;
}

static void qos_libc_heap_coalesce(void) {
    qos_libc_alloc_block_t *blk;
    blk = g_qos_heap_head;
    while (blk != 0 && blk->next != 0) {
        uint8_t *blk_end = (uint8_t *)(void *)(blk + 1) + blk->size;
        if (blk->free != 0 && blk->next->free != 0 && (uint8_t *)(void *)blk->next == blk_end) {
            blk->size += sizeof(qos_libc_alloc_block_t) + blk->next->size;
            blk->next = blk->next->next;
            continue;
        }
        blk = blk->next;
    }
}

static void qos_libc_file_init(void) {
    if (g_qos_file_init != 0) {
        return;
    }
    memset(g_qos_files, 0, sizeof(g_qos_files));
    g_qos_file_init = 1;
}

static qos_libc_file_t *qos_libc_file_alloc(void) {
    uint32_t i;
    qos_libc_file_init();
    for (i = 0; i < QOS_LIBC_FILE_MAX; i++) {
        if (g_qos_files[i].used == 0) {
            g_qos_files[i].used = 1;
            g_qos_files[i].backend = QOS_LIBC_BACKEND_HOST;
            g_qos_files[i].fd = -1;
            g_qos_files[i].mode = 0;
            return &g_qos_files[i];
        }
    }
    return 0;
}

static void qos_libc_file_free(qos_libc_file_t *file) {
    if (file == 0) {
        return;
    }
    file->used = 0;
    file->backend = QOS_LIBC_BACKEND_HOST;
    file->fd = -1;
    file->mode = 0;
}

static qos_libc_file_t *qos_libc_file_from_stream(FILE *stream) {
    uint32_t i;
    if (stream == 0) {
        return 0;
    }
    qos_libc_file_init();
    for (i = 0; i < QOS_LIBC_FILE_MAX; i++) {
        if (g_qos_files[i].used != 0 && (FILE *)(void *)&g_qos_files[i] == stream) {
            return &g_qos_files[i];
        }
    }
    return 0;
}

static int qos_libc_stdio_fd(FILE *stream) {
    if (stream == stdin) {
        return 0;
    }
    if (stream == stdout) {
        return 1;
    }
    if (stream == stderr) {
        return 2;
    }
    return -1;
}

static int qos_libc_mode_parse(const char *mode, uint32_t *out_mode, int *out_open_flags) {
    uint32_t m = 0;
    int open_flags = 0;
    if (mode == 0 || mode[0] == '\0' || out_mode == 0 || out_open_flags == 0) {
        return -1;
    }
    if (mode[0] == 'r') {
        m = QOS_LIBC_FILE_MODE_READ;
        open_flags = O_RDONLY;
    } else if (mode[0] == 'w') {
        m = QOS_LIBC_FILE_MODE_WRITE;
        open_flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (mode[0] == 'a') {
        m = QOS_LIBC_FILE_MODE_WRITE | QOS_LIBC_FILE_MODE_APPEND;
        open_flags = O_WRONLY | O_CREAT | O_APPEND;
    } else {
        return -1;
    }
    if (qos_strchr(mode, '+') != 0) {
        m |= QOS_LIBC_FILE_MODE_READ | QOS_LIBC_FILE_MODE_WRITE;
        open_flags &= ~(O_RDONLY | O_WRONLY);
        open_flags |= O_RDWR;
    }
    *out_mode = m;
    *out_open_flags = open_flags;
    return 0;
}

static int64_t qos_libc_fd_read(uint8_t backend, int fd, void *buf, uint32_t len) {
    int used = 0;
    int64_t rc = -1;
    if (backend == QOS_LIBC_BACKEND_KERNEL) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_READ, (uint64_t)(uint32_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len, 0, &used);
        if (used != 0) {
            return rc;
        }
    }
    return qos_libc_host_syscall6(SYS_read, fd, (long)(intptr_t)buf, len, 0, 0, 0);
}

static int64_t qos_libc_fd_write(uint8_t backend, int fd, const void *buf, uint32_t len) {
    int used = 0;
    int64_t rc = -1;
    if (backend == QOS_LIBC_BACKEND_KERNEL) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_WRITE, (uint64_t)(uint32_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len, 0, &used);
        if (used != 0) {
            return rc;
        }
    }
    return qos_libc_host_syscall6(SYS_write, fd, (long)(intptr_t)buf, len, 0, 0, 0);
}

static int64_t qos_libc_fd_close(uint8_t backend, int fd) {
    int used = 0;
    int64_t rc = -1;
    if (backend == QOS_LIBC_BACKEND_KERNEL) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_CLOSE, (uint64_t)(uint32_t)fd, 0, 0, 0, &used);
        if (used != 0) {
            return rc;
        }
    }
    return qos_libc_host_syscall6(SYS_close, fd, 0, 0, 0, 0, 0);
}

static int64_t qos_libc_fd_lseek(uint8_t backend, int fd, int64_t off, int whence) {
    int used = 0;
    int64_t rc = -1;
    if (backend == QOS_LIBC_BACKEND_KERNEL) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_LSEEK, (uint64_t)(uint32_t)fd, (uint64_t)off, (uint64_t)(uint32_t)whence, 0, &used);
        if (used != 0) {
            return rc;
        }
    }
    return qos_libc_host_syscall6(SYS_lseek, fd, (long)off, whence, 0, 0, 0);
}

static int64_t qos_libc_stdio_read(int fd, void *buf, uint32_t len) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_READ, (uint64_t)(uint32_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len, 0, &used);
    if (used != 0 && rc >= 0) {
        return rc;
    }
    return qos_libc_host_syscall6(SYS_read, fd, (long)(intptr_t)buf, len, 0, 0, 0);
}

static int64_t qos_libc_stdio_write(int fd, const void *buf, uint32_t len) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_WRITE, (uint64_t)(uint32_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len, 0, &used);
    if (used != 0 && rc >= 0) {
        return rc;
    }
    return qos_libc_host_syscall6(SYS_write, fd, (long)(intptr_t)buf, len, 0, 0, 0);
}

size_t qos_strlen(const char *s) {
    size_t n = 0;

    if (s == NULL) {
        return 0;
    }

    while (s[n] != '\0') {
        n++;
    }

    return n;
}

int qos_strcmp(const char *a, const char *b) {
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

int qos_strncmp(const char *a, const char *b, size_t n) {
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

char *qos_strcpy(char *dst, const char *src) {
    size_t i = 0;

    if (dst == NULL || src == NULL) {
        return dst;
    }

    while (1) {
        dst[i] = src[i];
        if (src[i] == '\0') {
            break;
        }
        i++;
    }

    return dst;
}

char *qos_strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;

    if (dst == NULL || src == NULL || n == 0) {
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

char *qos_strchr(const char *s, int c) {
    char target = (char)c;
    size_t i = 0;

    if (s == NULL) {
        return NULL;
    }

    while (1) {
        if (s[i] == target) {
            return (char *)(s + i);
        }
        if (s[i] == '\0') {
            break;
        }
        i++;
    }

    return NULL;
}

char *qos_strrchr(const char *s, int c) {
    char target = (char)c;
    const char *last = NULL;
    size_t i = 0;

    if (s == NULL) {
        return NULL;
    }

    while (1) {
        if (s[i] == target) {
            last = s + i;
        }
        if (s[i] == '\0') {
            break;
        }
        i++;
    }

    return (char *)last;
}

int qos_atoi(const char *s) {
    int sign = 1;
    int value = 0;
    size_t i = 0;

    if (s == NULL) {
        return 0;
    }

    while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r' || s[i] == '\f' || s[i] == '\v') {
        i++;
    }

    if (s[i] == '+' || s[i] == '-') {
        if (s[i] == '-') {
            sign = -1;
        }
        i++;
    }

    while (s[i] >= '0' && s[i] <= '9') {
        value = value * 10 + (int)(s[i] - '0');
        i++;
    }

    return sign * value;
}

uint16_t qos_htons(uint16_t v) {
    return (uint16_t)((v << 8) | (v >> 8));
}

uint32_t qos_htonl(uint32_t v) {
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) |
           ((v & 0xFF000000u) >> 24);
}

uint16_t qos_ntohs(uint16_t v) {
    return qos_htons(v);
}

uint32_t qos_ntohl(uint32_t v) {
    return qos_htonl(v);
}

uint32_t qos_inet_addr(const char *s) {
    uint32_t parts[4];
    uint32_t i = 0;
    uint32_t value = 0;

    if (s == NULL) {
        return 0xFFFFFFFFu;
    }

    while (*s != '\0' && i < 4) {
        if (*s < '0' || *s > '9') {
            return 0xFFFFFFFFu;
        }
        value = 0;
        while (*s >= '0' && *s <= '9') {
            value = value * 10u + (uint32_t)(*s - '0');
            if (value > 255u) {
                return 0xFFFFFFFFu;
            }
            s++;
        }
        parts[i++] = value;
        if (*s == '.') {
            s++;
        } else if (*s != '\0') {
            return 0xFFFFFFFFu;
        }
    }

    if (i != 4 || *s != '\0') {
        return 0xFFFFFFFFu;
    }

    return (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
}

char *qos_inet_ntoa(uint32_t addr, char *out, size_t out_len) {
    uint32_t a = (addr >> 24) & 0xFFu;
    uint32_t b = (addr >> 16) & 0xFFu;
    uint32_t c = (addr >> 8) & 0xFFu;
    uint32_t d = addr & 0xFFu;
    int wrote;

    if (out == NULL || out_len == 0) {
        return NULL;
    }
    wrote = qos_snprintf(out, out_len, "%u.%u.%u.%u", a, b, c, d);
    if (wrote < 0 || (size_t)wrote >= out_len) {
        return NULL;
    }
    out[out_len - 1] = '\0';
    return out;
}

void qos_errno_set(int err) {
    g_qos_errno = err;
}

int qos_errno_get(void) {
    return g_qos_errno;
}

const char *qos_strerror(int err) {
    switch (err) {
        case 0:
            return "Success";
        case 2:
            return "No such file or directory";
        case 9:
            return "Bad file descriptor";
        case 11:
            return "Resource temporarily unavailable";
        case 12:
            return "Out of memory";
        case 13:
            return "Permission denied";
        case 17:
            return "File exists";
        case 20:
            return "Not a directory";
        case 21:
            return "Is a directory";
        case 22:
            return "Invalid argument";
        case 32:
            return "Broken pipe";
        case 110:
            return "Connection timed out";
        case 111:
            return "Connection refused";
        default:
            return "Unknown error";
    }
}

void qos_perror(const char *prefix) {
    const char *msg = qos_strerror(g_qos_errno);
    char line[256];
    int wrote;
    if (prefix != NULL && prefix[0] != '\0') {
        wrote = qos_snprintf(line, sizeof(line), "%s: %s\n", prefix, msg);
    } else {
        wrote = qos_snprintf(line, sizeof(line), "%s\n", msg);
    }
    if (wrote > 0) {
        uint32_t len = (uint32_t)wrote;
        if ((size_t)len >= sizeof(line)) {
            len = (uint32_t)(sizeof(line) - 1u);
        }
        (void)qos_libc_stdio_write(2, line, len);
    }
}

int qos_snprintf(char *dst, size_t len, const char *fmt, ...) {
    int rc;
    va_list ap;
    if ((dst == NULL && len > 0) || fmt == NULL) {
        return -1;
    }
    va_start(ap, fmt);
    rc = qos_vsnprintf(dst, len, fmt, ap);
    va_end(ap);
    return rc;
}

int qos_getpid(void) {
    uint32_t pid = qos_libc_resolve_pid();
    if (pid != 0) {
        return (int)pid;
    }
    return (int)qos_libc_host_syscall6(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

static void qos_libc_fmt_putc(char **out, size_t *remaining, int *total, char ch) {
    if (remaining != 0 && *remaining > 1 && out != 0 && *out != 0) {
        **out = ch;
        (*out)++;
        (*remaining)--;
    }
    if (total != 0) {
        (*total)++;
    }
}

static void qos_libc_fmt_puts(char **out, size_t *remaining, int *total, const char *s) {
    size_t i = 0;
    const char *src = s != 0 ? s : "(null)";
    while (src[i] != '\0') {
        qos_libc_fmt_putc(out, remaining, total, src[i]);
        i++;
    }
}

static size_t qos_libc_u64_to_base(uint64_t value, uint32_t base, char *tmp, size_t cap, int upper) {
    static const char k_digits_low[] = "0123456789abcdef";
    static const char k_digits_up[] = "0123456789ABCDEF";
    const char *digits = upper != 0 ? k_digits_up : k_digits_low;
    size_t used = 0;
    if (tmp == 0 || cap == 0 || base < 2u || base > 16u) {
        return 0;
    }
    if (value == 0) {
        tmp[used++] = '0';
        return used;
    }
    while (value != 0 && used < cap) {
        tmp[used++] = digits[value % base];
        value /= base;
    }
    return used;
}

int qos_vsnprintf(char *dst, size_t len, const char *fmt, va_list ap) {
    char *out = dst;
    size_t remaining = len;
    int total = 0;
    size_t i = 0;
    if ((dst == NULL && len > 0) || fmt == NULL) {
        return -1;
    }
    if (len > 0) {
        dst[0] = '\0';
    }
    while (fmt[i] != '\0') {
        if (fmt[i] != '%') {
            qos_libc_fmt_putc(&out, &remaining, &total, fmt[i]);
            i++;
            continue;
        }
        i++;
        if (fmt[i] == '\0') {
            break;
        }
        if (fmt[i] == '%') {
            qos_libc_fmt_putc(&out, &remaining, &total, '%');
        } else if (fmt[i] == 'd') {
            int value = va_arg(ap, int);
            uint64_t mag;
            char tmp[32];
            size_t n;
            size_t j;
            if (value < 0) {
                qos_libc_fmt_putc(&out, &remaining, &total, '-');
                mag = (uint64_t)(-(int64_t)value);
            } else {
                mag = (uint64_t)value;
            }
            n = qos_libc_u64_to_base(mag, 10u, tmp, sizeof(tmp), 0);
            j = n;
            while (j > 0) {
                j--;
                qos_libc_fmt_putc(&out, &remaining, &total, tmp[j]);
            }
        } else if (fmt[i] == 'u') {
            uint32_t value = va_arg(ap, uint32_t);
            char tmp[32];
            size_t n = qos_libc_u64_to_base((uint64_t)value, 10u, tmp, sizeof(tmp), 0);
            size_t j = n;
            while (j > 0) {
                j--;
                qos_libc_fmt_putc(&out, &remaining, &total, tmp[j]);
            }
        } else if (fmt[i] == 'x') {
            uint32_t value = va_arg(ap, uint32_t);
            char tmp[32];
            size_t n = qos_libc_u64_to_base((uint64_t)value, 16u, tmp, sizeof(tmp), 0);
            size_t j = n;
            while (j > 0) {
                j--;
                qos_libc_fmt_putc(&out, &remaining, &total, tmp[j]);
            }
        } else if (fmt[i] == 'c') {
            int value = va_arg(ap, int);
            qos_libc_fmt_putc(&out, &remaining, &total, (char)value);
        } else if (fmt[i] == 's') {
            const char *value = va_arg(ap, const char *);
            qos_libc_fmt_puts(&out, &remaining, &total, value);
        } else if (fmt[i] == 'p') {
            uintptr_t value = (uintptr_t)va_arg(ap, void *);
            char tmp[32];
            size_t n;
            size_t j;
            qos_libc_fmt_putc(&out, &remaining, &total, '0');
            qos_libc_fmt_putc(&out, &remaining, &total, 'x');
            n = qos_libc_u64_to_base((uint64_t)value, 16u, tmp, sizeof(tmp), 0);
            j = n;
            while (j > 0) {
                j--;
                qos_libc_fmt_putc(&out, &remaining, &total, tmp[j]);
            }
        } else {
            qos_libc_fmt_putc(&out, &remaining, &total, '%');
            qos_libc_fmt_putc(&out, &remaining, &total, fmt[i]);
        }
        i++;
    }
    if (len > 0) {
        if (remaining > 0) {
            *out = '\0';
        } else {
            dst[len - 1u] = '\0';
        }
    }
    return total;
}

void *qos_memset(void *dst, int value, size_t len) {
    unsigned char *p = (unsigned char *)dst;
    size_t i = 0;

    while (i < len) {
        p[i] = (unsigned char)value;
        i++;
    }

    return dst;
}

int qos_memcmp(const void *a, const void *b, size_t len) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    size_t i = 0;

    while (i < len) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
        i++;
    }

    return 0;
}

void *qos_memcpy(void *dst, const void *src, size_t len) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    size_t i = 0;

    while (i < len) {
        d[i] = s[i];
        i++;
    }

    return dst;
}

void *qos_memmove(void *dst, const void *src, size_t len) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || len == 0) {
        return dst;
    }

    if (d < s) {
        size_t i = 0;
        while (i < len) {
            d[i] = s[i];
            i++;
        }
    } else {
        size_t i = len;
        while (i > 0) {
            i--;
            d[i] = s[i];
        }
    }

    return dst;
}

void *qos_malloc(size_t size) {
    qos_libc_alloc_block_t *blk;
    size_t need;
    if (size == 0) {
        return 0;
    }
    qos_libc_heap_init();
    need = qos_libc_align_up(size, QOS_LIBC_HEAP_ALIGN);
    blk = g_qos_heap_head;
    while (blk != 0) {
        if (blk->free != 0 && blk->size >= need) {
            qos_libc_heap_split(blk, need);
            blk->free = 0;
            return (void *)(blk + 1);
        }
        blk = blk->next;
    }
    return 0;
}

void qos_free(void *ptr) {
    qos_libc_alloc_block_t *blk;
    if (ptr == 0) {
        return;
    }
    blk = ((qos_libc_alloc_block_t *)ptr) - 1;
    blk->free = 1u;
    qos_libc_heap_coalesce();
}

void *qos_realloc(void *ptr, size_t size) {
    qos_libc_alloc_block_t *blk;
    void *next_ptr;
    size_t copy_len;
    if (ptr == 0) {
        return qos_malloc(size);
    }
    if (size == 0) {
        qos_free(ptr);
        return 0;
    }
    blk = ((qos_libc_alloc_block_t *)ptr) - 1;
    if (blk->size >= size) {
        return ptr;
    }
    next_ptr = qos_malloc(size);
    if (next_ptr == 0) {
        return 0;
    }
    copy_len = blk->size;
    memcpy(next_ptr, ptr, copy_len);
    qos_free(ptr);
    return next_ptr;
}

char *qos_strtok(char *str, const char *delim) {
    static __thread char *g_save = 0;
    char *start;
    char *p;
    if (delim == 0 || delim[0] == '\0') {
        return 0;
    }
    if (str != 0) {
        g_save = str;
    }
    if (g_save == 0) {
        return 0;
    }
    start = g_save;
    while (*start != '\0' && qos_strchr(delim, *start) != 0) {
        start++;
    }
    if (*start == '\0') {
        g_save = 0;
        return 0;
    }
    p = start;
    while (*p != '\0' && qos_strchr(delim, *p) == 0) {
        p++;
    }
    if (*p == '\0') {
        g_save = 0;
    } else {
        *p = '\0';
        g_save = p + 1;
    }
    return start;
}

int qos_printf(const char *fmt, ...) {
    char buf[1024];
    int rc;
    uint32_t emit_len;
    int64_t w;
    va_list ap;
    if (fmt == NULL) {
        return -1;
    }
    va_start(ap, fmt);
    rc = qos_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (rc < 0) {
        return -1;
    }
    emit_len = (uint32_t)rc;
    if ((size_t)emit_len >= sizeof(buf)) {
        emit_len = (uint32_t)(sizeof(buf) - 1u);
    }
    w = qos_libc_stdio_write(1, buf, emit_len);
    if (w < 0) {
        return -1;
    }
    return rc;
}

int qos_putchar(int c) {
    char ch = (char)c;
    return qos_libc_stdio_write(1, &ch, 1) == 1 ? (unsigned char)ch : -1;
}

int qos_puts(const char *s) {
    size_t len;
    int64_t w;
    if (s == 0) {
        return -1;
    }
    len = qos_strlen(s);
    w = qos_libc_stdio_write(1, s, (uint32_t)len);
    if (w < 0) {
        return -1;
    }
    if (qos_libc_stdio_write(1, "\n", 1) != 1) {
        return -1;
    }
    return (int)(len + 1u);
}

int qos_getchar(void) {
    char ch = 0;
    return qos_libc_stdio_read(0, &ch, 1) == 1 ? (unsigned char)ch : -1;
}

char *qos_fgets(char *s, int size, FILE *stream) {
    qos_libc_file_t *file;
    uint8_t backend;
    int fd;
    int i = 0;
    if (s == 0 || size <= 1 || stream == 0) {
        return 0;
    }
    file = qos_libc_file_from_stream(stream);
    if (file != 0) {
        backend = file->backend;
        fd = file->fd;
    } else {
        fd = qos_libc_stdio_fd(stream);
        if (fd < 0) {
            return 0;
        }
        backend = QOS_LIBC_BACKEND_HOST;
    }
    while (i < size - 1) {
        char ch = 0;
        int64_t rc = qos_libc_fd_read(backend, fd, &ch, 1);
        if (rc <= 0) {
            break;
        }
        s[i++] = ch;
        if (ch == '\n') {
            break;
        }
    }
    if (i == 0) {
        return 0;
    }
    s[i] = '\0';
    return s;
}

int qos_fputs(const char *s, FILE *stream) {
    qos_libc_file_t *file;
    uint8_t backend;
    int fd;
    size_t len;
    int64_t rc;
    if (s == 0 || stream == 0) {
        return -1;
    }
    file = qos_libc_file_from_stream(stream);
    if (file != 0) {
        backend = file->backend;
        fd = file->fd;
    } else {
        fd = qos_libc_stdio_fd(stream);
        if (fd < 0) {
            return -1;
        }
        backend = QOS_LIBC_BACKEND_HOST;
    }
    len = qos_strlen(s);
    rc = qos_libc_fd_write(backend, fd, s, (uint32_t)len);
    return rc < 0 ? -1 : (int)rc;
}

FILE *qos_fopen(const char *path, const char *mode) {
    qos_libc_file_t *file;
    uint32_t mode_bits = 0;
    int open_flags = 0;
    int used = 0;
    int64_t rc = -1;
    if (path == 0 || mode == 0 || qos_libc_mode_parse(mode, &mode_bits, &open_flags) != 0) {
        return 0;
    }
    file = qos_libc_file_alloc();
    if (file == 0) {
        return 0;
    }
    rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_OPEN, (uint64_t)(uintptr_t)path, (uint64_t)(uint32_t)open_flags, 0, 0, &used);
    if (used != 0 && rc < 0 && (mode_bits & QOS_LIBC_FILE_MODE_WRITE) != 0) {
        (void)qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_MKDIR, (uint64_t)(uintptr_t)path, 0, 0, 0, 0);
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_OPEN, (uint64_t)(uintptr_t)path, (uint64_t)(uint32_t)open_flags, 0, 0, &used);
    }
    if (used != 0 && rc >= 0) {
        file->backend = QOS_LIBC_BACKEND_KERNEL;
        file->fd = (int)rc;
    } else {
        rc = qos_libc_host_syscall6(SYS_openat, AT_FDCWD, (long)(intptr_t)path, open_flags, 0644, 0, 0);
        if (rc < 0) {
            qos_libc_file_free(file);
            return 0;
        }
        file->backend = QOS_LIBC_BACKEND_HOST;
        file->fd = (int)rc;
    }
    file->mode = mode_bits;
    if ((mode_bits & QOS_LIBC_FILE_MODE_APPEND) != 0) {
        (void)qos_libc_fd_lseek(file->backend, file->fd, 0, SEEK_END);
    }
    return (FILE *)(void *)file;
}

int qos_fclose(FILE *stream) {
    qos_libc_file_t *file = qos_libc_file_from_stream(stream);
    int64_t rc;
    if (file == 0) {
        if (qos_libc_stdio_fd(stream) >= 0) {
            return 0;
        }
        return -1;
    }
    rc = qos_libc_fd_close(file->backend, file->fd);
    qos_libc_file_free(file);
    return rc < 0 ? -1 : 0;
}

size_t qos_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    qos_libc_file_t *file;
    uint8_t backend;
    int fd;
    size_t want;
    int64_t rc;
    if (ptr == 0 || size == 0 || nmemb == 0 || stream == 0) {
        return 0;
    }
    if (nmemb > ((size_t)-1) / size) {
        return 0;
    }
    file = qos_libc_file_from_stream(stream);
    if (file != 0) {
        if ((file->mode & QOS_LIBC_FILE_MODE_READ) == 0) {
            return 0;
        }
        backend = file->backend;
        fd = file->fd;
    } else {
        fd = qos_libc_stdio_fd(stream);
        if (fd < 0) {
            return 0;
        }
        backend = QOS_LIBC_BACKEND_HOST;
    }
    want = size * nmemb;
    rc = qos_libc_fd_read(backend, fd, ptr, (uint32_t)want);
    if (rc <= 0) {
        return 0;
    }
    return ((size_t)rc) / size;
}

size_t qos_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    qos_libc_file_t *file;
    uint8_t backend;
    int fd;
    size_t want;
    int64_t rc;
    if (ptr == 0 || size == 0 || nmemb == 0 || stream == 0) {
        return 0;
    }
    if (nmemb > ((size_t)-1) / size) {
        return 0;
    }
    file = qos_libc_file_from_stream(stream);
    if (file != 0) {
        if ((file->mode & QOS_LIBC_FILE_MODE_WRITE) == 0) {
            return 0;
        }
        backend = file->backend;
        fd = file->fd;
    } else {
        fd = qos_libc_stdio_fd(stream);
        if (fd < 0) {
            return 0;
        }
        backend = QOS_LIBC_BACKEND_HOST;
    }
    want = size * nmemb;
    rc = qos_libc_fd_write(backend, fd, ptr, (uint32_t)want);
    if (rc <= 0) {
        return 0;
    }
    return ((size_t)rc) / size;
}

int qos_fseek(FILE *stream, long offset, int whence) {
    qos_libc_file_t *file = qos_libc_file_from_stream(stream);
    int64_t rc;
    if (file == 0) {
        return -1;
    }
    rc = qos_libc_fd_lseek(file->backend, file->fd, offset, whence);
    return rc < 0 ? -1 : 0;
}

long qos_ftell(FILE *stream) {
    qos_libc_file_t *file = qos_libc_file_from_stream(stream);
    int64_t rc;
    if (file == 0) {
        return -1;
    }
    rc = qos_libc_fd_lseek(file->backend, file->fd, 0, SEEK_CUR);
    return rc < 0 ? -1 : (long)rc;
}

int qos_fflush(FILE *stream) {
    if (stream == 0 || qos_libc_file_from_stream(stream) != 0 || qos_libc_stdio_fd(stream) >= 0) {
        return 0;
    }
    return -1;
}

int qos_fileno(FILE *stream) {
    qos_libc_file_t *file = qos_libc_file_from_stream(stream);
    if (file != 0) {
        return file->fd;
    }
    return qos_libc_stdio_fd(stream);
}

void qos_exit(int status) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    if (pid != 0) {
        (void)qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_EXIT, (uint64_t)pid, (uint64_t)(int32_t)status, 0, 0, &used);
    }
#if defined(SYS_exit_group)
    (void)qos_libc_host_syscall6(SYS_exit_group, status, 0, 0, 0, 0, 0);
#else
    (void)qos_libc_host_syscall6(SYS_exit, status, 0, 0, 0, 0, 0);
#endif
    for (;;) {
    }
}

void qos__exit(int status) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    if (pid != 0) {
        (void)qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_EXIT, (uint64_t)pid, (uint64_t)(int32_t)status, 0, 0, &used);
    }
    (void)qos_libc_host_syscall6(SYS_exit, status, 0, 0, 0, 0, 0);
    for (;;) {
    }
}

int qos_fork(void) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    uint32_t candidate;
    if (pid != 0) {
        for (candidate = 1; candidate <= 4096u; candidate++) {
            int64_t probe = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_GETPID, candidate, 0, 0, 0, &used);
            if (used != 0 && probe < 0) {
                rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_FORK, (uint64_t)pid, (uint64_t)candidate, 0, 0, &used);
                if (used != 0 && rc >= 0) {
                    return (int)rc;
                }
            }
        }
    }
#if defined(SYS_fork)
    rc = qos_libc_host_syscall6(SYS_fork, 0, 0, 0, 0, 0, 0);
#elif defined(SYS_clone)
    rc = qos_libc_host_syscall6(SYS_clone, SIGCHLD, 0, 0, 0, 0, 0);
#else
    rc = -1;
#endif
    return rc < 0 ? -1 : (int)rc;
}

int qos_execv(const char *path, char *const argv[]) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (pid != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_EXEC, (uint64_t)pid, (uint64_t)(uintptr_t)path, (uint64_t)(uintptr_t)argv, 0,
                               &used);
        if (used != 0) {
            return rc < 0 ? -1 : 0;
        }
    }
    rc = qos_libc_host_syscall6(SYS_execve, (long)(intptr_t)path, (long)(intptr_t)argv, (long)(intptr_t)environ, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

int qos_execve(const char *path, char *const argv[], char *const envp[]) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (pid != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_EXEC, (uint64_t)pid, (uint64_t)(uintptr_t)path, (uint64_t)(uintptr_t)argv, 0,
                               &used);
        if (used != 0) {
            return rc < 0 ? -1 : 0;
        }
    }
    rc = qos_libc_host_syscall6(SYS_execve, (long)(intptr_t)path, (long)(intptr_t)argv, (long)(intptr_t)envp, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

int qos_waitpid(int pid, int *status, int options) {
    uint32_t self = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (self != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_WAITPID, (uint64_t)self, (uint64_t)(int32_t)pid, (uint64_t)(uintptr_t)status,
                               (uint64_t)(uint32_t)options, &used);
        if (used != 0) {
            return rc < 0 ? -1 : (int)rc;
        }
    }
#if defined(SYS_wait4)
    rc = qos_libc_host_syscall6(SYS_wait4, pid, (long)(intptr_t)status, options, 0, 0, 0);
#else
    rc = -1;
#endif
    return rc < 0 ? -1 : (int)rc;
}

void qos_abort(void) {
    (void)qos_raise(SIGABRT);
    qos__exit(128 + SIGABRT);
}

int qos_socket(int domain, int type, int proto) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SOCKET, (uint64_t)domain, (uint64_t)type, (uint64_t)proto, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (int)rc;
    }
    rc = qos_libc_host_syscall6(SYS_socket, domain, type, proto, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

int qos_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_BIND, (uint64_t)fd, (uint64_t)(uintptr_t)addr, (uint64_t)addrlen, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (int)rc;
    }
    rc = qos_libc_host_syscall6(SYS_bind, fd, (long)(intptr_t)addr, addrlen, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

int qos_listen(int fd, int backlog) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_LISTEN, (uint64_t)fd, (uint64_t)backlog, 0, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (int)rc;
    }
    rc = qos_libc_host_syscall6(SYS_listen, fd, backlog, 0, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

int qos_accept(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_ACCEPT, (uint64_t)fd, (uint64_t)(uintptr_t)addr,
                                   (uint64_t)(uintptr_t)addrlen, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (int)rc;
    }
#if defined(SYS_accept)
    rc = qos_libc_host_syscall6(SYS_accept, fd, (long)(intptr_t)addr, (long)(intptr_t)addrlen, 0, 0, 0);
#elif defined(SYS_accept4)
    rc = qos_libc_host_syscall6(SYS_accept4, fd, (long)(intptr_t)addr, (long)(intptr_t)addrlen, 0, 0, 0);
#else
    rc = -1;
#endif
    return rc < 0 ? -1 : (int)rc;
}

int qos_connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_CONNECT, (uint64_t)fd, (uint64_t)(uintptr_t)addr, (uint64_t)addrlen, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (int)rc;
    }
    rc = qos_libc_host_syscall6(SYS_connect, fd, (long)(intptr_t)addr, addrlen, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

ssize_t qos_send(int fd, const void *buf, size_t len, int flags) {
    int used = 0;
    int64_t rc;
    rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SEND, (uint64_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (ssize_t)rc;
    }
    rc = qos_libc_host_syscall6(SYS_sendto, fd, (long)(intptr_t)buf, len, flags, 0, 0);
    return rc < 0 ? -1 : (ssize_t)rc;
}

ssize_t qos_recv(int fd, void *buf, size_t len, int flags) {
    int used = 0;
    int64_t rc;
    rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_RECV, (uint64_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (ssize_t)rc;
    }
    rc = qos_libc_host_syscall6(SYS_recvfrom, fd, (long)(intptr_t)buf, len, flags, 0, 0);
    return rc < 0 ? -1 : (ssize_t)rc;
}

ssize_t qos_sendto(int fd,
                   const void *buf,
                   size_t len,
                   int flags,
                   const struct sockaddr *addr,
                   socklen_t addrlen) {
    int used = 0;
    int64_t rc;
    rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SEND, (uint64_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len,
                           (uint64_t)(uintptr_t)addr, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (ssize_t)rc;
    }
    rc = qos_libc_host_syscall6(SYS_sendto, fd, (long)(intptr_t)buf, len, flags, (long)(intptr_t)addr, addrlen);
    return rc < 0 ? -1 : (ssize_t)rc;
}

ssize_t qos_recvfrom(int fd,
                     void *buf,
                     size_t len,
                     int flags,
                     struct sockaddr *addr,
                     socklen_t *addrlen) {
    int used = 0;
    int64_t rc;
    rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_RECV, (uint64_t)fd, (uint64_t)(uintptr_t)buf, (uint64_t)len,
                           (uint64_t)(uintptr_t)addr, &used);
    if (used != 0) {
        if (rc < 0) {
            return -1;
        }
        if (addrlen != NULL) {
            *addrlen = 16;
        }
        return (ssize_t)rc;
    }
    rc = qos_libc_host_syscall6(SYS_recvfrom, fd, (long)(intptr_t)buf, len, flags, (long)(intptr_t)addr, (long)(intptr_t)addrlen);
    if (rc < 0) {
        return -1;
    }
    return (ssize_t)rc;
}

int qos_close(int fd) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_CLOSE, (uint64_t)fd, 0, 0, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (int)rc;
    }
    rc = qos_libc_host_syscall6(SYS_close, fd, 0, 0, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

int qos_kill(int pid, int signum) {
    int used = 0;
    int64_t rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_KILL, (uint64_t)pid, (uint64_t)signum, 0, 0, &used);
    if (used != 0) {
        return rc < 0 ? -1 : (int)rc;
    }
    rc = qos_libc_host_syscall6(SYS_kill, pid, signum, 0, 0, 0, 0);
    return rc < 0 ? -1 : (int)rc;
}

void (*qos_signal(int signum, void (*handler)(int)))(int) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (pid != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGACTION, (uint64_t)pid, (uint64_t)(uint32_t)signum,
                               (uint64_t)(uintptr_t)handler, 0, &used);
        if (used != 0) {
            if (rc < 0) {
                return SIG_ERR;
            }
            return (void (*)(int))(uintptr_t)(uint64_t)rc;
        }
    }
    return SIG_ERR;
}

int qos_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    uint64_t new_handler = UINT64_MAX;
    if (pid != 0) {
        if (act != NULL) {
            new_handler = (uint64_t)(uintptr_t)act->sa_handler;
        }
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGACTION, (uint64_t)pid, (uint64_t)(uint32_t)signum, new_handler,
                               0, &used);
        if (used != 0) {
            if (rc < 0) {
                return -1;
            }
            if (oldact != NULL) {
                memset(oldact, 0, sizeof(*oldact));
                oldact->sa_handler = (void (*)(int))(uintptr_t)(uint64_t)rc;
            }
            return 0;
        }
    }
    return -1;
}

int qos_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (pid != 0) {
        if (set == NULL) {
            rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGPROCMASK, (uint64_t)pid, (uint64_t)SIG_BLOCK, 0, 0, &used);
        } else {
            rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGPROCMASK, (uint64_t)pid, (uint64_t)(uint32_t)how,
                                   (uint64_t)qos_libc_sigset_load(set), 0, &used);
        }
        if (used != 0) {
            if (rc < 0) {
                return -1;
            }
            qos_libc_sigset_store(oldset, (uint32_t)rc);
            return 0;
        }
    }
    return -1;
}

int qos_sigpending(sigset_t *set) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (pid != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGPENDING, (uint64_t)pid, 0, 0, 0, &used);
        if (used != 0) {
            if (rc < 0) {
                return -1;
            }
            qos_libc_sigset_store(set, (uint32_t)rc);
            return 0;
        }
    }
    return -1;
}

int qos_sigsuspend(const sigset_t *mask) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (pid != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGSUSPEND, (uint64_t)pid, (uint64_t)qos_libc_sigset_load(mask), 0,
                               0, &used);
        if (used != 0) {
            return rc < 0 ? -1 : (int)rc;
        }
    }
    return -1;
}

int qos_sigaltstack(const stack_t *ss, stack_t *old_ss) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    qos_libc_kernel_sigaltstack_t old_altstack;
    int have_old = 0;

    if (pid != 0) {
        memset(&old_altstack, 0, sizeof(old_altstack));
        if (old_ss != NULL && qos_proc_signal_altstack_get != 0) {
            if (qos_proc_signal_altstack_get(pid, &old_altstack) == 0) {
                have_old = 1;
            }
        }
        if (ss != NULL) {
            rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGALTSTACK, (uint64_t)pid, (uint64_t)(uintptr_t)ss->ss_sp,
                                   (uint64_t)ss->ss_size, (uint64_t)(uint32_t)ss->ss_flags, &used);
        } else {
            rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_SIGALTSTACK, (uint64_t)pid, UINT64_MAX, 0, 0, &used);
        }
        if (used != 0) {
            if (rc < 0) {
                return -1;
            }
            if (old_ss != NULL) {
                if (have_old == 0) {
                    old_altstack.flags = (uint32_t)rc;
                }
                old_ss->ss_sp = (void *)(uintptr_t)old_altstack.sp;
                old_ss->ss_size = (size_t)old_altstack.size;
                old_ss->ss_flags = (int)old_altstack.flags;
            }
            return 0;
        }
    }
    return -1;
}

int qos_raise(int signum) {
    uint32_t pid = qos_libc_resolve_pid();
    int used = 0;
    int64_t rc;
    if (pid != 0) {
        rc = qos_libc_syscall4(QOS_LIBC_SYSCALL_NR_KILL, (uint64_t)pid, (uint64_t)(uint32_t)signum, 0, 0, &used);
        if (used != 0) {
            return rc < 0 ? -1 : 0;
        }
    }
    rc = qos_libc_host_syscall6(SYS_kill, (long)qos_libc_host_syscall6(SYS_getpid, 0, 0, 0, 0, 0, 0), signum, 0, 0, 0, 0);
    return rc < 0 ? -1 : 0;
}
