#include <stdint.h>

#include "../irq/softirq.h"
#include "workqueue.h"

typedef struct {
    uint8_t used;
    uint8_t pending;
    uint32_t id;
    qos_workqueue_fn_t fn;
    void *arg;
} qos_workqueue_entry_t;

static qos_workqueue_entry_t g_entries[QOS_WORKQUEUE_MAX];
static uint16_t g_queue[QOS_WORKQUEUE_MAX];
static uint16_t g_qhead = 0;
static uint16_t g_qtail = 0;
static uint16_t g_qcount = 0;
static uint32_t g_next_id = 1;
static uint32_t g_completed = 0;

static int find_slot_by_id(uint32_t work_id) {
    uint32_t i = 0;
    while (i < QOS_WORKQUEUE_MAX) {
        if (g_entries[i].used != 0 && g_entries[i].id == work_id) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

static int queue_push(uint16_t slot) {
    if (g_qcount >= QOS_WORKQUEUE_MAX) {
        return -1;
    }
    g_queue[g_qtail] = slot;
    g_qtail = (uint16_t)((g_qtail + 1u) % QOS_WORKQUEUE_MAX);
    g_qcount++;
    return 0;
}

static int queue_pop(uint16_t *out_slot) {
    if (out_slot == 0 || g_qcount == 0) {
        return -1;
    }
    *out_slot = g_queue[g_qhead];
    g_qhead = (uint16_t)((g_qhead + 1u) % QOS_WORKQUEUE_MAX);
    g_qcount--;
    return 0;
}

static void workqueue_softirq_handler(void *ctx) {
    (void)ctx;
    while (g_qcount != 0) {
        uint16_t slot = 0;
        qos_workqueue_fn_t fn;
        void *arg;
        if (queue_pop(&slot) != 0) {
            return;
        }
        if (slot >= QOS_WORKQUEUE_MAX) {
            continue;
        }
        if (g_entries[slot].used == 0 || g_entries[slot].pending == 0 || g_entries[slot].fn == 0) {
            g_entries[slot].used = 0;
            g_entries[slot].pending = 0;
            g_entries[slot].id = 0;
            g_entries[slot].fn = 0;
            g_entries[slot].arg = 0;
            continue;
        }
        fn = g_entries[slot].fn;
        arg = g_entries[slot].arg;
        g_entries[slot].used = 0;
        g_entries[slot].pending = 0;
        g_entries[slot].id = 0;
        g_entries[slot].fn = 0;
        g_entries[slot].arg = 0;
        fn(arg);
        g_completed++;
    }
}

void qos_workqueue_reset(void) {
    uint32_t i = 0;
    while (i < QOS_WORKQUEUE_MAX) {
        g_entries[i].used = 0;
        g_entries[i].pending = 0;
        g_entries[i].id = 0;
        g_entries[i].fn = 0;
        g_entries[i].arg = 0;
        g_queue[i] = 0;
        i++;
    }
    g_qhead = 0;
    g_qtail = 0;
    g_qcount = 0;
    g_next_id = 1;
    g_completed = 0;
}

int qos_workqueue_enqueue(qos_workqueue_fn_t fn, void *arg, uint32_t *out_work_id) {
    uint32_t i = 0;
    if (fn == 0) {
        return -1;
    }
    while (i < QOS_WORKQUEUE_MAX) {
        if (g_entries[i].used == 0) {
            uint32_t id = g_next_id++;
            if (id == 0) {
                id = g_next_id++;
            }
            g_entries[i].used = 1;
            g_entries[i].pending = 1;
            g_entries[i].id = id;
            g_entries[i].fn = fn;
            g_entries[i].arg = arg;
            if (queue_push((uint16_t)i) != 0) {
                g_entries[i].used = 0;
                g_entries[i].pending = 0;
                g_entries[i].id = 0;
                g_entries[i].fn = 0;
                g_entries[i].arg = 0;
                return -1;
            }
            if (out_work_id != 0) {
                *out_work_id = id;
            }
            (void)qos_softirq_raise(QOS_SOFTIRQ_WORKQUEUE);
            return 0;
        }
        i++;
    }
    return -1;
}

int qos_workqueue_cancel(uint32_t work_id) {
    int slot;
    if (work_id == 0) {
        return -1;
    }
    slot = find_slot_by_id(work_id);
    if (slot < 0) {
        return -1;
    }
    g_entries[(uint32_t)slot].used = 0;
    g_entries[(uint32_t)slot].pending = 0;
    g_entries[(uint32_t)slot].id = 0;
    g_entries[(uint32_t)slot].fn = 0;
    g_entries[(uint32_t)slot].arg = 0;
    return 0;
}

uint32_t qos_workqueue_pending_count(void) {
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < QOS_WORKQUEUE_MAX; i++) {
        if (g_entries[i].used != 0 && g_entries[i].pending != 0) {
            count++;
        }
    }
    return count;
}

uint32_t qos_workqueue_completed_count(void) {
    return g_completed;
}

void workqueue_init(void) {
    qos_workqueue_reset();
    (void)qos_softirq_register(QOS_SOFTIRQ_WORKQUEUE, workqueue_softirq_handler, 0);
}
