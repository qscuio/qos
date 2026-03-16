/*
 * uffd.c — register a mapping with userfaultfd; handler thread serves faults
 *
 * Kernel path: handle_mm_fault -> handle_userfault
 * GDB:         break do_uffd_access
 * perf:        perf stat -e minor-faults ./uffd
 * build:       gcc -g -O0 -Wall -Wextra -o uffd uffd.c -lpthread
 * note:        uffd fd is blocking; handler thread reads() until close()
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define PAGES     8
#define PAGE_SIZE 4096

static volatile char sink;
static int    g_uffd;
static size_t g_len;

static void *fault_handler(void *arg)
{
    (void)arg;
    static char zero_page[PAGE_SIZE];   /* zero-filled by BSS */

    for (;;) {
        struct uffd_msg msg;
        ssize_t n = read(g_uffd, &msg, sizeof(msg));
        if (n == 0)
            break;
        if (n < 0) {
            perror("read uffd");
            break;
        }
        if (msg.event != UFFD_EVENT_PAGEFAULT)
            continue;

        unsigned long addr = msg.arg.pagefault.address & ~((unsigned long)PAGE_SIZE - 1);
        printf("  uffd fault at 0x%lx\n", addr);

        struct uffdio_copy copy = {
            .dst  = addr,
            .src  = (unsigned long)zero_page,
            .len  = PAGE_SIZE,
            .mode = 0,
        };
        if (ioctl(g_uffd, UFFDIO_COPY, &copy) < 0) {
            perror("UFFDIO_COPY");
            break;
        }
    }
    return NULL;
}

static void do_uffd_access(char *map, size_t len)
{
    /* Each read faults through the userfaultfd handler. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        sink = map[i];
}

int main(void)
{
    printf("uffd: register mapping, read triggers UFFD_EVENT_PAGEFAULT\n");
    printf("  kernel path : handle_mm_fault -> handle_userfault\n");
    printf("  GDB         : break do_uffd_access\n");
    printf("  perf        : perf stat -e minor-faults ./uffd\n\n");

    long fd = syscall(SYS_userfaultfd, O_CLOEXEC);
    if (fd < 0) {
        perror("userfaultfd");
        return 1;
    }
    g_uffd = (int)fd;

    struct uffdio_api api = { .api = UFFD_API };
    if (ioctl(g_uffd, UFFDIO_API, &api) < 0) {
        perror("UFFDIO_API");
        return 1;
    }

    size_t len  = (size_t)PAGES * PAGE_SIZE;
    g_len = len;
    char *map   = mmap(NULL, len, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    struct uffdio_register reg = {
        .range = { .start = (unsigned long)map, .len = len },
        .mode  = UFFDIO_REGISTER_MODE_MISSING,
    };
    if (ioctl(g_uffd, UFFDIO_REGISTER, &reg) < 0) {
        perror("UFFDIO_REGISTER");
        return 1;
    }

    pthread_t thr;
    pthread_create(&thr, NULL, fault_handler, NULL);

    printf("accessing %d registered pages...\n", PAGES);
    do_uffd_access(map, len);

    close(g_uffd);
    pthread_join(thr, NULL);

    printf("done: %d uffd faults handled\n", PAGES);
    munmap(map, len);
    return 0;
}
