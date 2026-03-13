#ifndef QOS_WORKQUEUE_H
#define QOS_WORKQUEUE_H

#include <stdint.h>

#define QOS_WORKQUEUE_MAX 128U

typedef void (*qos_workqueue_fn_t)(void *arg);

void qos_workqueue_reset(void);
int qos_workqueue_enqueue(qos_workqueue_fn_t fn, void *arg, uint32_t *out_work_id);
int qos_workqueue_cancel(uint32_t work_id);
uint32_t qos_workqueue_pending_count(void);
uint32_t qos_workqueue_completed_count(void);

void workqueue_init(void);

#endif
