#ifndef QOS_NET_NAPI_H
#define QOS_NET_NAPI_H

#include <stdint.h>

#define QOS_NAPI_MAX 32U

typedef uint32_t (*qos_napi_poll_t)(void *ctx, uint32_t budget);

void qos_napi_reset(void);
int qos_napi_register(uint32_t weight, qos_napi_poll_t poll, void *ctx, uint32_t *out_id);
int qos_napi_schedule(uint32_t napi_id);
int qos_napi_complete(uint32_t napi_id);
uint32_t qos_napi_pending_count(void);
uint32_t qos_napi_run_count(uint32_t napi_id);

void napi_init(void);

#endif
