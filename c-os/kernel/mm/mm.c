#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../init_state.h"
#include "../kernel.h"
#include "mm.h"

static uint64_t g_frames[QOS_PMM_MAX_PAGES];
static uint8_t g_used[QOS_PMM_MAX_PAGES];
static uint32_t g_frame_count = 0;
static uint32_t g_free_count = 0;
static uint64_t g_vas[QOS_VMM_MAX_AS][QOS_VMM_MAX_MAPPINGS];
static uint64_t g_pas[QOS_VMM_MAX_AS][QOS_VMM_MAX_MAPPINGS];
static uint32_t g_vflags[QOS_VMM_MAX_AS][QOS_VMM_MAX_MAPPINGS];
static uint8_t g_vused[QOS_VMM_MAX_AS][QOS_VMM_MAX_MAPPINGS];
static uint32_t g_vmm_asid = 0;

#if defined(__GNUC__)
extern uint8_t __kernel_phys_start __attribute__((weak));
extern uint8_t __kernel_phys_end __attribute__((weak));
#endif

static uint64_t align_up(uint64_t value) {
    return (value + (QOS_PAGE_SIZE - 1ULL)) & ~(QOS_PAGE_SIZE - 1ULL);
}

static uint64_t align_down(uint64_t value) {
    return value & ~(QOS_PAGE_SIZE - 1ULL);
}

void qos_pmm_reset(void) {
    memset(g_frames, 0, sizeof(g_frames));
    memset(g_used, 0, sizeof(g_used));
    g_frame_count = 0;
    g_free_count = 0;
}

void qos_vmm_reset(void) {
    memset(g_vas, 0, sizeof(g_vas));
    memset(g_pas, 0, sizeof(g_pas));
    memset(g_vflags, 0, sizeof(g_vflags));
    memset(g_vused, 0, sizeof(g_vused));
    g_vmm_asid = 0;
}

static void pmm_add_range(uint64_t base, uint64_t length) {
    uint64_t start = align_up(base);
    uint64_t end = align_down(base + length);

    while (start < end && g_frame_count < QOS_PMM_MAX_PAGES) {
        g_frames[g_frame_count] = start;
        g_used[g_frame_count] = 0;
        g_frame_count++;
        g_free_count++;
        start += QOS_PAGE_SIZE;
    }
}

static void pmm_reserve_range(uint64_t base, uint64_t length) {
    uint64_t end = base + length;
    uint32_t i = 0;

    if (length == 0) {
        return;
    }

    while (i < g_frame_count) {
        uint64_t frame = g_frames[i];
        if (frame >= base && frame < end && g_used[i] == 0) {
            g_used[i] = 1;
            g_free_count--;
        }
        i++;
    }
}

static void pmm_reserve_kernel_image(void) {
#if defined(__GNUC__)
    uint64_t start = (uint64_t)(uintptr_t)&__kernel_phys_start;
    uint64_t end = (uint64_t)(uintptr_t)&__kernel_phys_end;
    if (start != 0 && end > start) {
        pmm_reserve_range(start, end - start);
    }
#endif
}

int qos_pmm_init(const boot_info_t *boot_info) {
    uint32_t i = 0;

    qos_pmm_reset();

    if (boot_info == NULL || boot_info->magic != QOS_BOOT_MAGIC) {
        return -1;
    }

    if (boot_info->mmap_entry_count == 0 || boot_info->mmap_entry_count > QOS_MMAP_MAX_ENTRIES) {
        return -1;
    }

    while (i < boot_info->mmap_entry_count) {
        const mmap_entry_t *entry = &boot_info->mmap_entries[i];
        if (entry->type == 1U && entry->length >= QOS_PAGE_SIZE) {
            pmm_add_range(entry->base, entry->length);
        }
        i++;
    }

    if (g_frame_count == 0) {
        return -1;
    }

    pmm_reserve_range(0, 0x100000ULL);
    pmm_reserve_kernel_image();
    pmm_reserve_range((uint64_t)(uintptr_t)boot_info, sizeof(*boot_info));
    if (boot_info->initramfs_size != 0) {
        pmm_reserve_range(boot_info->initramfs_addr, boot_info->initramfs_size);
    }

    return 0;
}

uint32_t qos_pmm_total_pages(void) {
    return g_frame_count;
}

uint32_t qos_pmm_free_pages(void) {
    return g_free_count;
}

uint64_t qos_pmm_alloc_page(void) {
    uint32_t i = 0;

    while (i < g_frame_count) {
        if (g_used[i] == 0) {
            g_used[i] = 1;
            g_free_count--;
            return g_frames[i];
        }
        i++;
    }

    return QOS_PMM_PAGE_NONE;
}

int qos_pmm_free_page(uint64_t page) {
    uint32_t i = 0;

    if ((page & (QOS_PAGE_SIZE - 1ULL)) != 0ULL) {
        return -1;
    }

    while (i < g_frame_count) {
        if (g_frames[i] == page) {
            if (g_used[i] == 0) {
                return -1;
            }
            g_used[i] = 0;
            g_free_count++;
            return 0;
        }
        i++;
    }

    return -1;
}

static int asid_valid(uint32_t asid) {
    return asid < QOS_VMM_MAX_AS;
}

void qos_vmm_set_asid(uint32_t asid) {
    if (asid_valid(asid)) {
        g_vmm_asid = asid;
    }
}

uint32_t qos_vmm_get_asid(void) {
    return g_vmm_asid;
}

static int vmm_find(uint32_t asid, uint64_t page_va) {
    uint32_t i = 0;
    if (!asid_valid(asid)) {
        return -1;
    }
    while (i < QOS_VMM_MAX_MAPPINGS) {
        if (g_vused[asid][i] != 0 && g_vas[asid][i] == page_va) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

static int vmm_free_slot(uint32_t asid) {
    uint32_t i = 0;
    if (!asid_valid(asid)) {
        return -1;
    }
    while (i < QOS_VMM_MAX_MAPPINGS) {
        if (g_vused[asid][i] == 0) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

int qos_vmm_map_as(uint32_t asid, uint64_t va, uint64_t pa, uint32_t flags) {
    uint64_t page_va = align_down(va);
    int idx;

    if (!asid_valid(asid)) {
        return -1;
    }
    if (flags == 0) {
        return -1;
    }

    if ((va & (QOS_PAGE_SIZE - 1ULL)) != 0ULL || (pa & (QOS_PAGE_SIZE - 1ULL)) != 0ULL) {
        return -1;
    }

    idx = vmm_find(asid, page_va);
    if (idx < 0) {
        idx = vmm_free_slot(asid);
        if (idx < 0) {
            return -1;
        }
        g_vused[asid][(uint32_t)idx] = 1;
        g_vas[asid][(uint32_t)idx] = page_va;
    }

    g_pas[asid][(uint32_t)idx] = pa;
    g_vflags[asid][(uint32_t)idx] = flags;
#ifdef QOS_MM_DEBUG
    printf("[mm_debug] vmm_map_as: asid=%u VA=0x%lx -> PA=0x%lx PFN=0x%lx flags=0x%x\n",
           asid, (unsigned long)va, (unsigned long)pa,
           (unsigned long)(pa >> 12), flags);
#endif
    return 0;
}

int qos_vmm_map(uint64_t va, uint64_t pa, uint32_t flags) {
    return qos_vmm_map_as(g_vmm_asid, va, pa, flags);
}

int qos_vmm_unmap_as(uint32_t asid, uint64_t va) {
    uint64_t page_va = align_down(va);
    int idx;

    if (!asid_valid(asid)) {
        return -1;
    }
    if ((va & (QOS_PAGE_SIZE - 1ULL)) != 0ULL) {
        return -1;
    }

    idx = vmm_find(asid, page_va);
    if (idx < 0) {
        return -1;
    }

    g_vused[asid][(uint32_t)idx] = 0;
    g_vas[asid][(uint32_t)idx] = 0;
    g_pas[asid][(uint32_t)idx] = 0;
    g_vflags[asid][(uint32_t)idx] = 0;
    return 0;
}

int qos_vmm_unmap(uint64_t va) {
    return qos_vmm_unmap_as(g_vmm_asid, va);
}

uint64_t qos_vmm_translate_as(uint32_t asid, uint64_t va) {
    uint64_t page_va = align_down(va);
    uint64_t offset = va & (QOS_PAGE_SIZE - 1ULL);
    int idx;

    if (!asid_valid(asid)) {
        return 0;
    }
    idx = vmm_find(asid, page_va);
    if (idx < 0) {
        return 0;
    }

    return g_pas[asid][(uint32_t)idx] + offset;
}

uint64_t qos_vmm_translate(uint64_t va) {
    return qos_vmm_translate_as(g_vmm_asid, va);
}

uint32_t qos_vmm_flags_as(uint32_t asid, uint64_t va) {
    uint64_t page_va = align_down(va);
    int idx;

    if (!asid_valid(asid)) {
        return 0;
    }
    idx = vmm_find(asid, page_va);
    if (idx < 0) {
        return 0;
    }

    return g_vflags[asid][(uint32_t)idx];
}

uint32_t qos_vmm_flags(uint64_t va) {
    return qos_vmm_flags_as(g_vmm_asid, va);
}

uint32_t qos_vmm_mapping_count_as(uint32_t asid) {
    uint32_t i = 0;
    uint32_t count = 0;
    if (!asid_valid(asid)) {
        return 0;
    }
    while (i < QOS_VMM_MAX_MAPPINGS) {
        if (g_vused[asid][i] != 0) {
            count++;
        }
        i++;
    }
    return count;
}

uint32_t qos_vmm_mapping_count(void) {
    return qos_vmm_mapping_count_as(g_vmm_asid);
}

int qos_vmm_mapping_get_as(uint32_t asid, uint32_t ordinal, uint64_t *out_va, uint64_t *out_pa, uint32_t *out_flags) {
    uint32_t i = 0;
    uint32_t seen = 0;
    if (!asid_valid(asid) || out_va == 0 || out_pa == 0 || out_flags == 0) {
        return -1;
    }
    while (i < QOS_VMM_MAX_MAPPINGS) {
        if (g_vused[asid][i] != 0) {
            if (seen == ordinal) {
                *out_va = g_vas[asid][i];
                *out_pa = g_pas[asid][i];
                *out_flags = g_vflags[asid][i];
                return 0;
            }
            seen++;
        }
        i++;
    }
    return -1;
}

int qos_vmm_mapping_get(uint32_t ordinal, uint64_t *out_va, uint64_t *out_pa, uint32_t *out_flags) {
    return qos_vmm_mapping_get_as(g_vmm_asid, ordinal, out_va, out_pa, out_flags);
}

void mm_init(void) {
    qos_kernel_state_mark(QOS_INIT_MM);
}
