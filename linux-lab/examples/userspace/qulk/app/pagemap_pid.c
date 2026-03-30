#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>

#define PAGEMAP_LENGTH 8 // Pagemap entry length is fixed at 8 bytes on 64-bit systems

uint64_t get_pagemap_entry(int pagemap_fd, uintptr_t vaddr, long page_size) {
    uint64_t offset = (vaddr / page_size) * PAGEMAP_LENGTH;
    uint64_t entry;

    if (lseek(pagemap_fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    if (read(pagemap_fd, &entry, PAGEMAP_LENGTH) != PAGEMAP_LENGTH) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    return entry;
}

uintptr_t get_physical_address(int pagemap_fd, uintptr_t vaddr, long page_size) {
    uint64_t entry = get_pagemap_entry(pagemap_fd, vaddr, page_size);

    // Print the entry for debugging
    printf("Pagemap entry for vaddr 0x%" PRIxPTR ": 0x%016" PRIx64 "\n", vaddr, entry);

    if (!(entry & (1ULL << 63))) {
        printf("Page not present\n");
        return 0; // Page not present
    }

    if (entry & (1ULL << 62)) {
        printf("Page is swapped\n");
        return 0; // Page is swapped, no physical address
    }

    uint64_t pfn = entry & ((1ULL << 55) - 1);
    return (pfn * page_size) + (vaddr % page_size);
}

void dump_memory_mapping(pid_t pid, long page_size) {
    char maps_file[256], pagemap_file[256];
    snprintf(maps_file, sizeof(maps_file), "/proc/%d/maps", pid);
    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%d/pagemap", pid);

    FILE *maps_fp = fopen(maps_file, "r");
    if (!maps_fp) {
        perror("fopen maps file");
        exit(EXIT_FAILURE);
    }

    int pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) {
        perror("open pagemap file");
        fclose(maps_fp);
        exit(EXIT_FAILURE);
    }

    char line[256];
    uintptr_t start, end;
    while (fgets(line, sizeof(line), maps_fp)) {
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &start, &end) == 2) {
            for (uintptr_t addr = start; addr < end; addr += page_size) {
                uintptr_t paddr = get_physical_address(pagemap_fd, addr, page_size);
                if (paddr) {
                    printf("0x%016" PRIxPTR " -> 0x%016" PRIxPTR "\n", addr, paddr);
                }
            }
        }
    }

    close(pagemap_fd);
    fclose(maps_fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = atoi(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "Invalid PID: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf");
        exit(EXIT_FAILURE);
    }

    printf("Page size: %ld bytes\n", page_size);

    dump_memory_mapping(pid, page_size);

    return 0;
}

