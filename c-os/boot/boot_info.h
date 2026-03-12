#ifndef QOS_BOOT_INFO_H
#define QOS_BOOT_INFO_H

#include <stdint.h>

#define QOS_BOOT_MAGIC 0x514F53424F4F5400ULL
#define QOS_MMAP_MAX_ENTRIES 128U

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t _pad;
} mmap_entry_t;

typedef struct {
    uint64_t magic;

    uint32_t mmap_entry_count;
    uint32_t _pad0;
    mmap_entry_t mmap_entries[QOS_MMAP_MAX_ENTRIES];

    uint64_t fb_addr;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
    uint8_t fb_bpp;
    uint8_t _pad1[3];

    uint64_t initramfs_addr;
    uint64_t initramfs_size;

    uint64_t acpi_rsdp_addr;
    uint64_t dtb_addr;
} boot_info_t;

#endif
