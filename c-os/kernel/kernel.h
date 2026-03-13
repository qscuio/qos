#ifndef QOS_KERNEL_H
#define QOS_KERNEL_H

#include <stdint.h>

#include "../boot/boot_info.h"
#include "drivers/drivers.h"
#include "fs/vfs.h"
#include "irq/softirq.h"
#include "kthread/kthread.h"
#include "mm/mm.h"
#include "net/net.h"
#include "net/napi.h"
#include "proc/proc.h"
#include "sched/sched.h"
#include "syscall/syscall.h"
#include "timer/timer.h"
#include "workqueue/workqueue.h"

enum {
    QOS_INIT_MM = 1u << 0,
    QOS_INIT_DRIVERS = 1u << 1,
    QOS_INIT_VFS = 1u << 2,
    QOS_INIT_NET = 1u << 3,
    QOS_INIT_SCHED = 1u << 4,
    QOS_INIT_SYSCALL = 1u << 5,
    QOS_INIT_PROC = 1u << 6,
};

#define QOS_INIT_ALL \
    (QOS_INIT_MM | QOS_INIT_DRIVERS | QOS_INIT_VFS | QOS_INIT_NET | QOS_INIT_SCHED | QOS_INIT_SYSCALL | \
     QOS_INIT_PROC)

typedef struct {
    uint32_t init_state;
    uint32_t pmm_total_pages;
    uint32_t pmm_free_pages;
    uint32_t sched_count;
    uint32_t vfs_count;
    uint32_t net_queue_len;
    uint32_t drivers_count;
    uint32_t syscall_count;
    uint32_t proc_count;
} qos_kernel_status_t;

int qos_boot_info_validate(const boot_info_t *boot_info);
int qos_kernel_entry(const boot_info_t *boot_info);
void qos_kernel_reset(void);
uint32_t qos_kernel_init_state(void);
int qos_kernel_status_snapshot(qos_kernel_status_t *out);

#endif
