#ifndef QOS_SCHED_H
#define QOS_SCHED_H

#include <stdint.h>

#define QOS_SCHED_MAX_TASKS 64U
#define QOS_SCHED_LEVELS 4U
#define QOS_SCHED_PRIO_IDLE 0U
#define QOS_SCHED_PRIO_LOW 1U
#define QOS_SCHED_PRIO_NORMAL 2U
#define QOS_SCHED_PRIO_HIGH 3U

void qos_sched_reset(void);
int qos_sched_add_task(uint32_t pid);
int qos_sched_add_task_prio(uint32_t pid, uint32_t priority);
int qos_sched_remove_task(uint32_t pid);
int qos_sched_set_priority(uint32_t pid, uint32_t priority);
int qos_sched_get_priority(uint32_t pid, uint32_t *out_priority);
uint32_t qos_sched_count(void);
uint32_t qos_sched_next(void);

void sched_init(void);

#endif
