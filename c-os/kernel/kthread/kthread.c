#include <stdint.h>
#include <string.h>

#include "kthread.h"

typedef struct {
    uint8_t used;
    uint8_t runnable;
    uint32_t tid;
    qos_kthread_fn_t entry;
    void *arg;
    uint32_t run_count;
} qos_kthread_t;

static qos_kthread_t g_threads[QOS_KTHREAD_MAX];
static char g_names[QOS_KTHREAD_MAX][QOS_KTHREAD_NAME_MAX];
static uint8_t g_name_len[QOS_KTHREAD_MAX];
static uint32_t g_next_tid = 1;
static uint32_t g_cursor = 0;
static uint32_t g_current_tid = 0;

static uint32_t write_u32_ascii(char *out, uint32_t cap, uint32_t value) {
    char tmp[16];
    uint32_t n = 0;
    uint32_t i = 0;

    if (out == 0 || cap == 0) {
        return 0;
    }
    if (value == 0) {
        if (cap > 1) {
            out[0] = '0';
        }
        return 1;
    }
    while (value != 0 && n < (uint32_t)sizeof(tmp)) {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (i < n && i + 1 < cap) {
        out[i] = tmp[n - 1u - i];
        i++;
    }
    return n;
}

static void init_name_slot(uint32_t slot, uint32_t tid) {
    static const char prefix[] = "kthread-";
    uint32_t off = 0;
    uint32_t i = 0;
    uint32_t n;

    while (i + 1 < (uint32_t)sizeof(prefix) && off + 1 < QOS_KTHREAD_NAME_MAX) {
        g_names[slot][off++] = prefix[i++];
    }
    n = write_u32_ascii(g_names[slot] + off, QOS_KTHREAD_NAME_MAX - off, tid);
    off += n;
    if (off >= QOS_KTHREAD_NAME_MAX) {
        off = QOS_KTHREAD_NAME_MAX - 1U;
    }
    g_names[slot][off] = '\0';
    g_name_len[slot] = (uint8_t)off;
}

static int find_slot_by_tid(uint32_t tid) {
    uint32_t i = 0;
    while (i < QOS_KTHREAD_MAX) {
        if (g_threads[i].used != 0 && g_threads[i].tid == tid) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

void qos_kthread_reset(void) {
    uint32_t i = 0;
    while (i < QOS_KTHREAD_MAX) {
        g_threads[i].used = 0;
        g_threads[i].runnable = 0;
        g_threads[i].tid = 0;
        g_threads[i].entry = 0;
        g_threads[i].arg = 0;
        g_threads[i].run_count = 0;
        g_names[i][0] = '\0';
        g_name_len[i] = 0;
        i++;
    }
    g_next_tid = 1;
    g_cursor = 0;
    g_current_tid = 0;
}

int qos_kthread_create(qos_kthread_fn_t entry, void *arg, uint32_t *out_tid) {
    uint32_t i = 0;
    if (entry == 0) {
        return -1;
    }
    while (i < QOS_KTHREAD_MAX) {
        if (g_threads[i].used == 0) {
            uint32_t tid = g_next_tid++;
            if (tid == 0) {
                tid = g_next_tid++;
            }
            g_threads[i].used = 1;
            g_threads[i].runnable = 1;
            g_threads[i].tid = tid;
            g_threads[i].entry = entry;
            g_threads[i].arg = arg;
            g_threads[i].run_count = 0;
            init_name_slot(i, tid);
            if (out_tid != 0) {
                *out_tid = tid;
            }
            return 0;
        }
        i++;
    }
    return -1;
}

int qos_kthread_wake(uint32_t tid) {
    int slot = find_slot_by_tid(tid);
    if (slot < 0) {
        return -1;
    }
    g_threads[(uint32_t)slot].runnable = 1;
    return 0;
}

int qos_kthread_stop(uint32_t tid) {
    int slot = find_slot_by_tid(tid);
    if (slot < 0) {
        return -1;
    }
    g_threads[(uint32_t)slot].runnable = 0;
    return 0;
}

uint32_t qos_kthread_count(void) {
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < QOS_KTHREAD_MAX; i++) {
        if (g_threads[i].used != 0) {
            count++;
        }
    }
    return count;
}

uint32_t qos_kthread_run_count(uint32_t tid) {
    int slot = find_slot_by_tid(tid);
    if (slot < 0) {
        return 0;
    }
    return g_threads[(uint32_t)slot].run_count;
}

uint32_t qos_kthread_run_next(void) {
    uint32_t scanned = 0;
    while (scanned < QOS_KTHREAD_MAX) {
        uint32_t idx = (g_cursor + scanned) % QOS_KTHREAD_MAX;
        if (g_threads[idx].used != 0 && g_threads[idx].runnable != 0 && g_threads[idx].entry != 0) {
            uint32_t tid = g_threads[idx].tid;
            qos_kthread_fn_t fn = g_threads[idx].entry;
            void *arg = g_threads[idx].arg;
            g_threads[idx].run_count++;
            g_cursor = (idx + 1U) % QOS_KTHREAD_MAX;
            g_current_tid = tid;
            fn(arg);
            return tid;
        }
        scanned++;
    }
    g_current_tid = 0;
    return 0;
}

uint32_t qos_kthread_current_tid(void) {
    return g_current_tid;
}

int32_t qos_kthread_name_get(uint32_t tid, char *out, uint32_t out_len) {
    int slot = find_slot_by_tid(tid);
    uint32_t len;
    if (slot < 0 || out == 0 || out_len == 0) {
        return -1;
    }
    len = (uint32_t)g_name_len[(uint32_t)slot];
    if (len + 1 > out_len) {
        return -1;
    }
    memcpy(out, g_names[(uint32_t)slot], len + 1);
    return (int32_t)len;
}

void kthread_init(void) {
    qos_kthread_reset();
}
