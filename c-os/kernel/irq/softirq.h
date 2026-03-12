#ifndef QOS_IRQ_SOFTIRQ_H
#define QOS_IRQ_SOFTIRQ_H

#include <stdint.h>

#define QOS_SOFTIRQ_MAX 8U
#define QOS_SOFTIRQ_TIMER 0U
#define QOS_SOFTIRQ_NET_RX 1U

typedef void (*qos_softirq_handler_t)(void *ctx);

void qos_softirq_reset(void);
int qos_softirq_register(uint32_t vector, qos_softirq_handler_t handler, void *ctx);
int qos_softirq_raise(uint32_t vector);
uint32_t qos_softirq_pending_mask(void);
uint32_t qos_softirq_run(void);

void softirq_init(void);

#endif
