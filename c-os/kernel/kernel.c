#include "init_state.h"
#include "kernel.h"

int qos_boot_info_validate(const boot_info_t *boot_info) {
    if (boot_info == 0) {
        return -1;
    }

    if (boot_info->magic != QOS_BOOT_MAGIC) {
        return -1;
    }

    if (boot_info->mmap_entry_count == 0 || boot_info->mmap_entry_count > QOS_MMAP_MAX_ENTRIES) {
        return -1;
    }

    if (boot_info->mmap_entries[0].length == 0 || boot_info->mmap_entries[0].type == 0) {
        return -1;
    }

    if (boot_info->initramfs_size != 0 && boot_info->initramfs_addr == 0) {
        return -1;
    }

    return 0;
}

void qos_kernel_reset(void) {
    qos_kernel_state_reset();
    qos_vmm_reset();
    qos_syscall_reset();
    qos_proc_reset();
}

uint32_t qos_kernel_init_state(void) {
    return qos_kernel_state_get();
}

int qos_kernel_status_snapshot(qos_kernel_status_t *out) {
    if (out == 0) {
        return -1;
    }

    out->init_state = qos_kernel_init_state();
    out->pmm_total_pages = qos_pmm_total_pages();
    out->pmm_free_pages = qos_pmm_free_pages();
    out->sched_count = qos_sched_count();
    out->vfs_count = qos_vfs_count();
    out->net_queue_len = qos_net_queue_len();
    out->drivers_count = qos_drivers_count();
    out->syscall_count = qos_syscall_count();
    out->proc_count = qos_proc_count();
    return 0;
}

int qos_kernel_entry(const boot_info_t *boot_info) {
    qos_kernel_reset();
    if (qos_boot_info_validate(boot_info) != 0) {
        return -1;
    }

    if (qos_pmm_init(boot_info) != 0) {
        return -1;
    }

    mm_init();
    drivers_init();
    vfs_init();
    net_init();
    sched_init();
    syscall_init();
    proc_init();

    if (qos_kernel_init_state() != QOS_INIT_ALL) {
        return -1;
    }

    return 0;
}
