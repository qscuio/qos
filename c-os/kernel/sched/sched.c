#include "../init_state.h"
#include "../kernel.h"
#include "sched.h"

static uint32_t g_queues[QOS_SCHED_LEVELS][QOS_SCHED_MAX_TASKS];
static uint32_t g_counts[QOS_SCHED_LEVELS];
static uint32_t g_rr_cursor[QOS_SCHED_LEVELS];
static uint32_t g_total_count = 0;
static uint32_t g_current_pid = 0;

static int priority_valid(uint32_t priority) {
    return priority <= QOS_SCHED_PRIO_HIGH;
}

static int find_pid_in_priority(uint32_t priority, uint32_t pid) {
    uint32_t i = 0;
    if (!priority_valid(priority)) {
        return -1;
    }
    while (i < g_counts[priority]) {
        if (g_queues[priority][i] == pid) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

static int find_pid(uint32_t pid, uint32_t *out_priority, uint32_t *out_index) {
    uint32_t priority = 0;
    while (priority < QOS_SCHED_LEVELS) {
        int idx = find_pid_in_priority(priority, pid);
        if (idx >= 0) {
            if (out_priority != 0) {
                *out_priority = priority;
            }
            if (out_index != 0) {
                *out_index = (uint32_t)idx;
            }
            return 0;
        }
        priority++;
    }
    return -1;
}

static int queue_append(uint32_t priority, uint32_t pid) {
    uint32_t count;

    if (!priority_valid(priority)) {
        return -1;
    }
    count = g_counts[priority];
    if (count >= QOS_SCHED_MAX_TASKS) {
        return -1;
    }
    g_queues[priority][count] = pid;
    g_counts[priority] = count + 1U;
    g_total_count++;
    return 0;
}

static int queue_insert_at(uint32_t priority, uint32_t index, uint32_t pid) {
    uint32_t count;
    uint32_t i;

    if (!priority_valid(priority)) {
        return -1;
    }
    count = g_counts[priority];
    if (count >= QOS_SCHED_MAX_TASKS) {
        return -1;
    }
    if (index > count) {
        index = count;
    }

    i = count;
    while (i > index) {
        g_queues[priority][i] = g_queues[priority][i - 1U];
        i--;
    }
    g_queues[priority][index] = pid;
    g_counts[priority] = count + 1U;
    if (index < g_rr_cursor[priority]) {
        g_rr_cursor[priority]++;
    }
    g_total_count++;
    return 0;
}

static void queue_remove_at(uint32_t priority, uint32_t index) {
    uint32_t i;

    if (!priority_valid(priority) || index >= g_counts[priority]) {
        return;
    }

    i = index;
    while (i + 1U < g_counts[priority]) {
        g_queues[priority][i] = g_queues[priority][i + 1U];
        i++;
    }
    g_counts[priority]--;
    g_queues[priority][g_counts[priority]] = 0;
    g_total_count--;

    if (g_counts[priority] == 0) {
        g_rr_cursor[priority] = 0;
        return;
    }
    if (g_rr_cursor[priority] >= g_counts[priority]) {
        g_rr_cursor[priority] = 0;
    }
    if (index < g_rr_cursor[priority] && g_rr_cursor[priority] > 0) {
        g_rr_cursor[priority]--;
    }
}

void qos_sched_reset(void) {
    uint32_t i = 0;
    while (i < QOS_SCHED_LEVELS) {
        uint32_t j = 0;
        while (j < QOS_SCHED_MAX_TASKS) {
            g_queues[i][j] = 0;
            j++;
        }
        g_counts[i] = 0;
        g_rr_cursor[i] = 0;
        i++;
    }
    g_total_count = 0;
    g_current_pid = 0;
}

int qos_sched_add_task(uint32_t pid) {
    return qos_sched_add_task_prio(pid, QOS_SCHED_PRIO_NORMAL);
}

int qos_sched_add_task_prio(uint32_t pid, uint32_t priority) {
    if (pid == 0 || !priority_valid(priority)) {
        return -1;
    }

    if (find_pid(pid, 0, 0) == 0) {
        return -1;
    }

    if (g_total_count >= QOS_SCHED_MAX_TASKS) {
        return -1;
    }

    return queue_append(priority, pid);
}

int qos_sched_remove_task(uint32_t pid) {
    uint32_t priority = 0;
    uint32_t index = 0;

    if (find_pid(pid, &priority, &index) != 0) {
        return -1;
    }

    queue_remove_at(priority, index);
    return 0;
}

int qos_sched_set_priority(uint32_t pid, uint32_t priority) {
    uint32_t old_priority = 0;
    uint32_t old_index = 0;

    if (!priority_valid(priority)) {
        return -1;
    }

    if (find_pid(pid, &old_priority, &old_index) != 0) {
        return -1;
    }

    if (old_priority == priority) {
        return 0;
    }

    queue_remove_at(old_priority, old_index);
    if (queue_append(priority, pid) != 0) {
        (void)queue_insert_at(old_priority, old_index, pid);
        return -1;
    }
    return 0;
}

int qos_sched_get_priority(uint32_t pid, uint32_t *out_priority) {
    uint32_t priority = 0;
    if (out_priority == 0 || find_pid(pid, &priority, 0) != 0) {
        return -1;
    }

    *out_priority = priority;
    return 0;
}

uint32_t qos_sched_count(void) {
    return g_total_count;
}

uint32_t qos_sched_next(void) {
    uint32_t level = QOS_SCHED_LEVELS;

    if (g_total_count == 0) {
        g_current_pid = 0;
        return 0;
    }

    while (level > 0) {
        uint32_t priority = level - 1U;
        uint32_t count = g_counts[priority];
        if (count != 0) {
            uint32_t idx = g_rr_cursor[priority] % count;
            uint32_t pid = g_queues[priority][idx];
            g_rr_cursor[priority] = (idx + 1U) % count;
            g_current_pid = pid;
            return pid;
        }
        level--;
    }

    g_current_pid = 0;
    return 0;
}

uint32_t qos_sched_current(void) {
    return g_current_pid;
}

void sched_init(void) {
    qos_sched_reset();
    qos_kernel_state_mark(QOS_INIT_SCHED);
}
