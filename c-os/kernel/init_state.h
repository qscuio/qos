#ifndef QOS_KERNEL_INIT_STATE_H
#define QOS_KERNEL_INIT_STATE_H

#include <stdint.h>

void qos_kernel_state_reset(void);
void qos_kernel_state_mark(uint32_t bit);
uint32_t qos_kernel_state_get(void);

#endif
