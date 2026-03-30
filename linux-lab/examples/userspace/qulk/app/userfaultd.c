/* userfaultfd_demo.c

          Licensed under the GNU General Public License version 2 or later.
       */
#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <poll.h>

/*
 * userfaultfd 是 Linux 内核提供的一种机制，允许用户空间进程处理页面错误。传统上，页面错误由内核处理，当程序访问未映射的内存区域时，内核会自动分配物理内存并更新页表。但有些情况下，我们希望用户空间代码能够介入这个过程，比如实现用户空间的分页机制、内存快照恢复或者高效的虚拟内存管理。userfaultfd 就是为这种需求而设计的。
 *
 * userfaultfd 的主要功能
 * 延迟分配内存：用户空间进程可以在页面错误发生时决定如何分配或填充内存。
 * 拦截和处理页面错误：可以实现自己的页面调度策略。
 * 内存迁移：在不停止进程的情况下，可以将内存内容从一个位置迁移到另一个位置。
 * 使用步骤
 * 创建 userfaultfd 文件描述符：
 * 使用 userfaultfd 系统调用创建一个文件描述符。
 * 设置监视范围：
 * 使用 ioctl 命令来设置需要监视的地址范围。
 * 处理页面错误：
 * 用户空间进程通过 poll 或 select 系统调用等待页面错误事件，并在事件发生时读取错误信息。
 * 填充内存：
 * 用户空间进程通过 mmap 或 copy 等系统调用来填充缺页。
 */

#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

static int page_size;

static void *
fault_handler_thread(void *arg)
{
    static struct uffd_msg msg; /* Data read from userfaultfd */
    static int fault_cnt = 0;   /* Number of faults so far handled */
    long uffd;                  /* userfaultfd file descriptor */
    static char *page = NULL;
    struct uffdio_copy uffdio_copy;
    ssize_t nread;

    uffd = (long)arg;

    /* Create a page that will be copied into the faulting region */

    if (page == NULL)
    {
        page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (page == MAP_FAILED)
            errExit("mmap");
    }

    /* Loop, handling incoming events on the userfaultfd
              file descriptor */

    for (;;)
    {

        /* See what poll() tells us about the userfaultfd */

        struct pollfd pollfd;
        int nready;
        pollfd.fd = uffd;
        pollfd.events = POLLIN;
        nready = poll(&pollfd, 1, -1);
        if (nready == -1)
            errExit("poll");

        printf("\nfault_handler_thread():\n");
        printf("    poll() returns: nready = %d; "
               "POLLIN = %d; POLLERR = %d\n",
               nready,
               (pollfd.revents & POLLIN) != 0,
               (pollfd.revents & POLLERR) != 0);

        /* Read an event from the userfaultfd */

        nread = read(uffd, &msg, sizeof(msg));
        if (nread == 0)
        {
            printf("EOF on userfaultfd!\n");
            exit(EXIT_FAILURE);
        }

        if (nread == -1)
            errExit("read");

        /* We expect only one kind of event; verify that assumption */

        if (msg.event != UFFD_EVENT_PAGEFAULT)
        {
            fprintf(stderr, "Unexpected event on userfaultfd\n");
            exit(EXIT_FAILURE);
        }

        /* Display info about the page-fault event */

        printf("    UFFD_EVENT_PAGEFAULT event: ");
        printf("flags = %llx; ", msg.arg.pagefault.flags);
        printf("address = %llx\n", msg.arg.pagefault.address);

        /* Copy the page pointed to by 'page' into the faulting
                  region. Vary the contents that are copied in, so that it
                  is more obvious that each fault is handled separately. */

        memset(page, 'A' + fault_cnt % 20, page_size);
        fault_cnt++;

        uffdio_copy.src = (unsigned long)page;

        /* We need to handle page faults in units of pages(!).
                  So, round faulting address down to page boundary */

        uffdio_copy.dst = (unsigned long)msg.arg.pagefault.address &
                          ~(page_size - 1);
        uffdio_copy.len = page_size;
        uffdio_copy.mode = 0;
        uffdio_copy.copy = 0;
        if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
            errExit("ioctl-UFFDIO_COPY");

        printf("        (uffdio_copy.copy returned %lld)\n",
               uffdio_copy.copy);
    }
}

int main(int argc, char *argv[])
{
    long uffd;         /* userfaultfd file descriptor */
    char *addr;        /* Start of region handled by userfaultfd */
    unsigned long len; /* Length of region handled by userfaultfd */
    pthread_t thr;     /* ID of thread that handles page faults */
    struct uffdio_api uffdio_api;
    struct uffdio_register uffdio_register;
    int s;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s num-pages\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    page_size = sysconf(_SC_PAGE_SIZE);
    len = strtoul(argv[1], NULL, 0) * page_size;

    /* Create and enable userfaultfd object */

    uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd == -1)
        errExit("userfaultfd");

    uffdio_api.api = UFFD_API;
    uffdio_api.features = 0;
    if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
        errExit("ioctl-UFFDIO_API");

    /* Create a private anonymous mapping. The memory will be
              demand-zero paged--that is, not yet allocated. When we
              actually touch the memory, it will be allocated via
              the userfaultfd. */

    addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
        errExit("mmap");

    printf("Address returned by mmap() = %p\n", addr);

    /* Register the memory range of the mapping we just created for
              handling by the userfaultfd object. In mode, we request to track
              missing pages (i.e., pages that have not yet been faulted in). */

    uffdio_register.range.start = (unsigned long)addr;
    uffdio_register.range.len = len;
    uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
    if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
        errExit("ioctl-UFFDIO_REGISTER");

    /* Create a thread that will process the userfaultfd events */

    s = pthread_create(&thr, NULL, fault_handler_thread, (void *)uffd);
    if (s != 0)
    {
        errno = s;
        errExit("pthread_create");
    }

    /* Main thread now touches memory in the mapping, touching
              locations 1024 bytes apart. This will trigger userfaultfd
              events for all pages in the region. */

    int l;
    l = 0xf; /* Ensure that faulting address is not on a page
                          boundary, in order to test that we correctly
                          handle that case in fault_handling_thread() */
    while (l < len)
    {
        char c = addr[l];
        printf("Read address %p in main(): ", addr + l);
        printf("%c\n", c);
        l += 1024;
        usleep(100000); /* Slow things down a little */
    }

    exit(EXIT_SUCCESS);
}
