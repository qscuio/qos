#include <stdint.h>

#include "../irq/softirq.h"
#include "napi.h"

typedef struct {
    uint8_t used;
    uint8_t scheduled;
    uint32_t id;
    uint32_t weight;
    qos_napi_poll_t poll;
    void *ctx;
    uint32_t run_count;
} qos_napi_t;

static qos_napi_t g_napi[QOS_NAPI_MAX];
static uint32_t g_next_napi_id = 1;

static int find_slot(uint32_t napi_id) {
    uint32_t i = 0;
    while (i < QOS_NAPI_MAX) {
        if (g_napi[i].used != 0 && g_napi[i].id == napi_id) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

static void napi_softirq_handler(void *ctx) {
    uint32_t i = 0;
    int need_reschedule = 0;
    (void)ctx;

    while (i < QOS_NAPI_MAX) {
        if (g_napi[i].used != 0 && g_napi[i].scheduled != 0 && g_napi[i].poll != 0) {
            uint32_t weight = g_napi[i].weight == 0 ? 1u : g_napi[i].weight;
            uint32_t work = g_napi[i].poll(g_napi[i].ctx, weight);
            g_napi[i].run_count++;
            if (work < weight) {
                g_napi[i].scheduled = 0;
            } else {
                need_reschedule = 1;
            }
        }
        i++;
    }

    if (need_reschedule != 0) {
        (void)qos_softirq_raise(QOS_SOFTIRQ_NET_RX);
    }
}

void qos_napi_reset(void) {
    uint32_t i = 0;
    while (i < QOS_NAPI_MAX) {
        g_napi[i].used = 0;
        g_napi[i].scheduled = 0;
        g_napi[i].id = 0;
        g_napi[i].weight = 0;
        g_napi[i].poll = 0;
        g_napi[i].ctx = 0;
        g_napi[i].run_count = 0;
        i++;
    }
    g_next_napi_id = 1;
}

int qos_napi_register(uint32_t weight, qos_napi_poll_t poll, void *ctx, uint32_t *out_id) {
    uint32_t i = 0;
    if (poll == 0 || weight == 0) {
        return -1;
    }
    while (i < QOS_NAPI_MAX) {
        if (g_napi[i].used == 0) {
            uint32_t id = g_next_napi_id++;
            if (id == 0) {
                id = g_next_napi_id++;
            }
            g_napi[i].used = 1;
            g_napi[i].scheduled = 0;
            g_napi[i].id = id;
            g_napi[i].weight = weight;
            g_napi[i].poll = poll;
            g_napi[i].ctx = ctx;
            g_napi[i].run_count = 0;
            if (out_id != 0) {
                *out_id = id;
            }
            return 0;
        }
        i++;
    }
    return -1;
}

int qos_napi_schedule(uint32_t napi_id) {
    int slot = find_slot(napi_id);
    if (slot < 0) {
        return -1;
    }
    g_napi[(uint32_t)slot].scheduled = 1;
    return qos_softirq_raise(QOS_SOFTIRQ_NET_RX);
}

int qos_napi_complete(uint32_t napi_id) {
    int slot = find_slot(napi_id);
    if (slot < 0) {
        return -1;
    }
    g_napi[(uint32_t)slot].scheduled = 0;
    return 0;
}

uint32_t qos_napi_pending_count(void) {
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < QOS_NAPI_MAX; i++) {
        if (g_napi[i].used != 0 && g_napi[i].scheduled != 0) {
            count++;
        }
    }
    return count;
}

uint32_t qos_napi_run_count(uint32_t napi_id) {
    int slot = find_slot(napi_id);
    if (slot < 0) {
        return 0;
    }
    return g_napi[(uint32_t)slot].run_count;
}

void napi_init(void) {
    qos_napi_reset();
    (void)qos_softirq_register(QOS_SOFTIRQ_NET_RX, napi_softirq_handler, 0);
}
