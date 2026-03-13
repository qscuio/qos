#ifndef QOS_KTHREAD_H
#define QOS_KTHREAD_H

#include <stdint.h>

#define QOS_KTHREAD_MAX 64U
#define QOS_KTHREAD_NAME_MAX 32U

typedef void (*qos_kthread_fn_t)(void *arg);

void qos_kthread_reset(void);
int qos_kthread_create(qos_kthread_fn_t entry, void *arg, uint32_t *out_tid);
int qos_kthread_wake(uint32_t tid);
int qos_kthread_stop(uint32_t tid);
uint32_t qos_kthread_count(void);
uint32_t qos_kthread_run_count(uint32_t tid);
uint32_t qos_kthread_run_next(void);
uint32_t qos_kthread_current_tid(void);
int32_t qos_kthread_name_get(uint32_t tid, char *out, uint32_t out_len);

void kthread_init(void);

#endif
