/*
 * zero_page.c — trigger zero-page faults via madvise(MADV_DONTNEED) + re-read
 *
 * Kernel path: handle_mm_fault -> do_anonymous_page (zero page)
 * GDB:         break do_zero_page_read
 * perf:        perf stat -e minor-faults ./zero_page
 */
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGES     64
#define PAGE_SIZE 4096

static volatile char sink;

static void do_zero_page_read(char *map, size_t len)
{
    /*
     * After MADV_DONTNEED the kernel drops physical pages. Re-reading
     * each page faults in the shared zero page.
     */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        sink = map[i];
}

int main(void)
{
    size_t len = (size_t)PAGES * PAGE_SIZE;

    printf("zero_page: mmap, write, madvise(MADV_DONTNEED), re-read\n");
    printf("  kernel path : handle_mm_fault -> do_anonymous_page (zero page)\n");
    printf("  GDB         : break do_zero_page_read\n");
    printf("  perf        : perf stat -e minor-faults ./zero_page\n\n");

    char *map = mmap(NULL, len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* Materialize pages so MADV_DONTNEED has something to release. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = 0xFF;

    printf("releasing pages with MADV_DONTNEED...\n");
    if (madvise(map, len, MADV_DONTNEED) < 0) {
        perror("madvise");
        return 1;
    }

    printf("re-reading (faults in zero pages)...\n");
    do_zero_page_read(map, len);

    printf("re-read %d pages — check minor-faults count with perf\n", PAGES);
    munmap(map, len);
    return 0;
}
