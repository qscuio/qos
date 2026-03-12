#ifndef QOS_MM_H
#define QOS_MM_H

#include <stdint.h>

#include "../../boot/boot_info.h"

#define QOS_PAGE_SIZE 4096ULL
#define QOS_PMM_MAX_PAGES 4096U
#define QOS_PMM_PAGE_NONE UINT64_MAX
#define QOS_VMM_MAX_MAPPINGS 2048U
#define QOS_VMM_MAX_AS 64U

enum {
    VM_READ = 1u << 0,
    VM_WRITE = 1u << 1,
    VM_EXEC = 1u << 2,
    VM_USER = 1u << 3,
    VM_COW = 1u << 4,
};

int qos_pmm_init(const boot_info_t *boot_info);
void qos_pmm_reset(void);
uint32_t qos_pmm_total_pages(void);
uint32_t qos_pmm_free_pages(void);
uint64_t qos_pmm_alloc_page(void);
int qos_pmm_free_page(uint64_t page);

void qos_vmm_reset(void);
void qos_vmm_set_asid(uint32_t asid);
uint32_t qos_vmm_get_asid(void);
int qos_vmm_map(uint64_t va, uint64_t pa, uint32_t flags);
int qos_vmm_unmap(uint64_t va);
uint64_t qos_vmm_translate(uint64_t va);
uint32_t qos_vmm_flags(uint64_t va);
int qos_vmm_map_as(uint32_t asid, uint64_t va, uint64_t pa, uint32_t flags);
int qos_vmm_unmap_as(uint32_t asid, uint64_t va);
uint64_t qos_vmm_translate_as(uint32_t asid, uint64_t va);
uint32_t qos_vmm_flags_as(uint32_t asid, uint64_t va);

void mm_init(void);

#endif
