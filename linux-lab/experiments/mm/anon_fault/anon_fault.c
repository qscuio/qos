/*
 * anon_fault.c — trigger anonymous page faults via mmap + first write
 *
 * Kernel path: handle_mm_fault -> do_anonymous_page
 * GDB:         break do_anon_fault
 * perf:        perf stat -e minor-faults ./anon_fault
 */
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGES     64
#define PAGE_SIZE 4096

static volatile char sink;

static void do_anon_fault(char *map, size_t len)
{
    /* Writing each page for the first time faults in a new anonymous page. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = (char)i;
    sink = map[0];
}

int main(void)
{
    size_t len = (size_t)PAGES * PAGE_SIZE;

    printf("anon_fault: mmap %zu bytes, write each page\n", len);
    printf("  kernel path : handle_mm_fault -> do_anonymous_page\n");
    printf("  GDB         : break do_anon_fault\n");
    printf("  perf        : perf stat -e minor-faults ./anon_fault\n\n");

    char *map = mmap(NULL, len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    do_anon_fault(map, len);

    printf("touched %d pages — check minor-faults count with perf\n", PAGES);
    munmap(map, len);
    return 0;
}
