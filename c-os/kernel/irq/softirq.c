#include <stddef.h>
#include <stdint.h>

#include "../init_state.h"
#include "../kernel.h"
#include "softirq.h"

static qos_softirq_handler_t g_handlers[QOS_SOFTIRQ_MAX];
static void *g_handler_ctx[QOS_SOFTIRQ_MAX];
static uint32_t g_pending_mask = 0;

static int vector_valid(uint32_t vector) {
    return vector < QOS_SOFTIRQ_MAX;
}

void qos_softirq_reset(void) {
    uint32_t i = 0;
    while (i < QOS_SOFTIRQ_MAX) {
        g_handlers[i] = 0;
        g_handler_ctx[i] = 0;
        i++;
    }
    g_pending_mask = 0;
}

int qos_softirq_register(uint32_t vector, qos_softirq_handler_t handler, void *ctx) {
    if (!vector_valid(vector) || handler == 0) {
        return -1;
    }
    g_handlers[vector] = handler;
    g_handler_ctx[vector] = ctx;
    return 0;
}

int qos_softirq_raise(uint32_t vector) {
    if (!vector_valid(vector)) {
        return -1;
    }
    g_pending_mask |= (1u << vector);
    return 0;
}

uint32_t qos_softirq_pending_mask(void) {
    return g_pending_mask;
}

uint32_t qos_softirq_run(void) {
    uint32_t handled = 0;
    while (g_pending_mask != 0) {
        uint32_t i = 0;
        while (i < QOS_SOFTIRQ_MAX) {
            uint32_t bit = (1u << i);
            if ((g_pending_mask & bit) != 0) {
                qos_softirq_handler_t fn = g_handlers[i];
                void *ctx = g_handler_ctx[i];
                g_pending_mask &= ~bit;
                if (fn != 0) {
                    fn(ctx);
                    handled++;
                }
                break;
            }
            i++;
        }
    }
    return handled;
}

void softirq_init(void) {
    qos_softirq_reset();
}
