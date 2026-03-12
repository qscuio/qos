#ifndef QOS_TIMER_H
#define QOS_TIMER_H

#include <stdint.h>

#define QOS_TIMER_MAX 128U

typedef void (*qos_timer_callback_t)(void *ctx);

void qos_timer_reset(void);
uint64_t qos_timer_jiffies(void);
int32_t qos_timer_add(uint32_t delay_ticks, uint32_t interval_ticks, qos_timer_callback_t callback, void *ctx);
int qos_timer_cancel(int32_t timer_id);
void qos_timer_tick(uint32_t elapsed_ticks);
uint32_t qos_timer_active_count(void);
uint32_t qos_timer_pending_count(void);

void timer_init(void);

#endif
