// SPDX-License-Identifier: GPL-2.0
/*
 * va_to_pa.c — translate virtual addresses to physical addresses via
 * /proc/self/pagemap, demonstrating demand paging and copy-on-write.
 *
 * Requires sudo (CAP_SYS_ADMIN) — the PFN field in pagemap entries is
 * restricted since kernel 4.0.
 *
 * Kernel paths exercised:
 *   heap touch before/after: do_anonymous_page()
 *   child write after fork:  do_wp_page()
 *
 * Usage:
 *   gcc -O0 -o va_to_pa va_to_pa.c && sudo ./va_to_pa
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGE_SIZE  4096UL
#define PAGE_SHIFT 12

/* /proc/self/pagemap entry layout (64 bits per page) */
typedef union {
    uint64_t raw;
    struct {
        uint64_t pfn        : 55; /* bits 54:0 — PFN when present */
        uint64_t soft_dirty : 1;  /* bit 55 */
        uint64_t exclusive  : 1;  /* bit 56 */
        uint64_t _unused    : 4;  /* bits 60:57 */
        uint64_t file_mapped: 1;  /* bit 61 */
        uint64_t swapped    : 1;  /* bit 62 */
        uint64_t present    : 1;  /* bit 63 */
    };
} pagemap_entry_t;

static uint64_t va_to_pa(const void *va)
{
    int fd;
    uint64_t vpn, offset;
    pagemap_entry_t entry;
    uint64_t pa;

    fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0) {
        perror("  open /proc/self/pagemap");
        return 0;
    }

    vpn    = (uint64_t)(uintptr_t)va >> PAGE_SHIFT;
    offset = vpn * sizeof(uint64_t);

    if (pread(fd, &entry, sizeof(entry), (off_t)offset) != sizeof(entry)) {
        if (errno == EPERM) {
            fprintf(stderr, "  ERROR: /proc/self/pagemap PFN read requires "
                    "CAP_SYS_ADMIN — run with sudo\n");
        } else {
            perror("  pread pagemap");
        }
        close(fd);
        return 0;
    }
    close(fd);

    if (!entry.present) {
        printf("  page NOT present (not yet faulted in by kernel)\n");
        return 0;
    }

    pa = (entry.pfn << PAGE_SHIFT) | ((uint64_t)(uintptr_t)va & (PAGE_SIZE - 1));
    return pa;
}

int main(void)
{
    uint64_t pa, parent_pa, child_pa_before, child_pa_after;
    char *heap;
    pid_t pid;

    /* ── 1. Stack variable (always present) ───────────────────────── */
    int x = 42;
    printf("\n[1] stack variable x=%d\n", x);
    printf("    VA  = %p\n", (void *)&x);
    pa = va_to_pa(&x);
    if (pa)
        printf("    PA  = 0x%lx  PFN = 0x%lx\n", (unsigned long)pa,
               (unsigned long)(pa >> PAGE_SHIFT));

    /* ── 2. Heap page before first touch ──────────────────────────── */
    heap = malloc(PAGE_SIZE);
    if (!heap) { perror("malloc"); return 1; }

    printf("\n[2] heap page — before first touch\n");
    printf("    VA  = %p\n", (void *)heap);
    va_to_pa(heap);

    /* ── 3. Heap page after first write — do_anonymous_page fires ─── */
    heap[0] = (char)0xAB;

    printf("\n[3] heap page — after first write (do_anonymous_page fired)\n");
    printf("    VA  = %p\n", (void *)heap);
    pa = va_to_pa(heap);
    if (pa)
        printf("    PA  = 0x%lx  PFN = 0x%lx  byte@PA = 0x%02x\n",
               (unsigned long)pa, (unsigned long)(pa >> PAGE_SHIFT),
               (unsigned char)heap[0]);

    /* ── 4. Fork — same VA, same PA until child writes ────────────── */
    printf("\n[4] fork — CoW demonstration\n");
    heap[0] = (char)0x11;
    parent_pa = va_to_pa(heap);
    printf("    parent: VA=%p  PA=0x%lx  val=0x%02x\n",
           (void *)heap, (unsigned long)parent_pa, (unsigned char)heap[0]);

    pid = fork();
    if (pid < 0) { perror("fork"); free(heap); return 1; }

    if (pid == 0) {
        child_pa_before = va_to_pa(heap);
        printf("    child (before write): VA=%p  PA=0x%lx  same_as_parent=%s\n",
               (void *)heap, (unsigned long)child_pa_before,
               child_pa_before == parent_pa ? "YES" : "NO");

        heap[0] = (char)0x22;

        child_pa_after = va_to_pa(heap);
        printf("    child (after  write): VA=%p  PA=0x%lx  new_page=%s\n",
               (void *)heap, (unsigned long)child_pa_after,
               child_pa_after != parent_pa ? "YES" : "NO");
        free(heap);
        _exit(0);
    }

    wait(NULL);
    free(heap);
    return 0;
}
