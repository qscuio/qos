/*
 * cow_fault.c — trigger copy-on-write faults via fork + child write
 *
 * Kernel path: handle_mm_fault -> do_wp_page
 * GDB:         break do_cow_write
 * perf:        perf stat -e minor-faults ./cow_fault
 */
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGES     64
#define PAGE_SIZE 4096

static volatile char sink;

static void do_cow_write(char *map, size_t len)
{
    /* Each write breaks the shared CoW mapping, triggering do_wp_page. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = 0xAB;
    sink = map[0];
}

int main(void)
{
    size_t len = (size_t)PAGES * PAGE_SIZE;

    printf("cow_fault: mmap, fill in parent, fork, child writes all pages\n");
    printf("  kernel path : handle_mm_fault -> do_wp_page\n");
    printf("  GDB         : break do_cow_write\n");
    printf("  perf        : perf stat -e minor-faults ./cow_fault\n\n");

    char *map = mmap(NULL, len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* Materialize pages in the parent so they are shared after fork. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = 0x01;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        do_cow_write(map, len);
        printf("child: wrote %d pages (CoW fault per page)\n", PAGES);
        return 0;
    }

    int status;
    waitpid(pid, &status, 0);
    printf("parent: child exited — check minor-faults count with perf\n");
    munmap(map, len);
    return 0;
}
