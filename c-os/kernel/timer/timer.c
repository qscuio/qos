#include <stddef.h>
#include <stdint.h>

#include "../irq/softirq.h"
#include "timer.h"

typedef struct {
    uint8_t used;
    uint8_t periodic;
    uint8_t pending;
    uint64_t expires;
    uint32_t interval;
    qos_timer_callback_t callback;
    void *ctx;
} qos_timer_entry_t;

static qos_timer_entry_t g_timers[QOS_TIMER_MAX];
static uint64_t g_jiffies = 0;

static int timer_id_valid(int32_t timer_id) {
    return timer_id > 0 && (uint32_t)timer_id <= QOS_TIMER_MAX;
}

static void timer_softirq_handler(void *ctx) {
    uint32_t i = 0;
    uint64_t now = g_jiffies;
    (void)ctx;
    while (i < QOS_TIMER_MAX) {
        qos_timer_entry_t *t = &g_timers[i];
        if (t->used != 0 && t->pending != 0 && t->expires <= now) {
            qos_timer_callback_t cb = t->callback;
            void *cb_ctx = t->ctx;
            t->pending = 0;
            if (cb != 0) {
                cb(cb_ctx);
            }
            if (t->used != 0) {
                if (t->periodic != 0 && t->interval != 0) {
                    while (t->expires <= now) {
                        t->expires += (uint64_t)t->interval;
                    }
                } else {
                    t->used = 0;
                    t->callback = 0;
                    t->ctx = 0;
                    t->interval = 0;
                    t->expires = 0;
                }
            }
        }
        i++;
    }
}

void qos_timer_reset(void) {
    uint32_t i = 0;
    while (i < QOS_TIMER_MAX) {
        g_timers[i].used = 0;
        g_timers[i].periodic = 0;
        g_timers[i].pending = 0;
        g_timers[i].expires = 0;
        g_timers[i].interval = 0;
        g_timers[i].callback = 0;
        g_timers[i].ctx = 0;
        i++;
    }
    g_jiffies = 0;
}

uint64_t qos_timer_jiffies(void) {
    return g_jiffies;
}

int32_t qos_timer_add(uint32_t delay_ticks, uint32_t interval_ticks, qos_timer_callback_t callback, void *ctx) {
    uint32_t i = 0;
    if (delay_ticks == 0 || callback == 0) {
        return -1;
    }
    while (i < QOS_TIMER_MAX) {
        if (g_timers[i].used == 0) {
            g_timers[i].used = 1;
            g_timers[i].periodic = interval_ticks != 0 ? 1 : 0;
            g_timers[i].pending = 0;
            g_timers[i].interval = interval_ticks;
            g_timers[i].expires = g_jiffies + (uint64_t)delay_ticks;
            g_timers[i].callback = callback;
            g_timers[i].ctx = ctx;
            return (int32_t)(i + 1U);
        }
        i++;
    }
    return -1;
}

int qos_timer_cancel(int32_t timer_id) {
    uint32_t idx;
    if (!timer_id_valid(timer_id)) {
        return -1;
    }
    idx = (uint32_t)(timer_id - 1);
    if (g_timers[idx].used == 0) {
        return -1;
    }
    g_timers[idx].used = 0;
    g_timers[idx].periodic = 0;
    g_timers[idx].pending = 0;
    g_timers[idx].expires = 0;
    g_timers[idx].interval = 0;
    g_timers[idx].callback = 0;
    g_timers[idx].ctx = 0;
    return 0;
}

void qos_timer_tick(uint32_t elapsed_ticks) {
    uint32_t i;
    int need_softirq = 0;
    if (elapsed_ticks == 0) {
        return;
    }
    g_jiffies += (uint64_t)elapsed_ticks;
    for (i = 0; i < QOS_TIMER_MAX; i++) {
        if (g_timers[i].used != 0 && g_timers[i].pending == 0 && g_timers[i].expires <= g_jiffies) {
            g_timers[i].pending = 1;
            need_softirq = 1;
        }
    }
    if (need_softirq != 0) {
        (void)qos_softirq_raise(QOS_SOFTIRQ_TIMER);
    }
}

uint32_t qos_timer_active_count(void) {
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < QOS_TIMER_MAX; i++) {
        if (g_timers[i].used != 0) {
            count++;
        }
    }
    return count;
}

uint32_t qos_timer_pending_count(void) {
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < QOS_TIMER_MAX; i++) {
        if (g_timers[i].used != 0 && g_timers[i].pending != 0) {
            count++;
        }
    }
    return count;
}

void timer_init(void) {
    qos_timer_reset();
    (void)qos_softirq_register(QOS_SOFTIRQ_TIMER, timer_softirq_handler, 0);
}
