#include "trace_toolkit.h"
#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <linux/version.h>
#include <trace/events/sched.h>

/* Global variable for tracepoint lookup */
static struct tracepoint *tp_ret;

/* Tracepoint lookup callback */
static void tracepoint_lookup(struct tracepoint *tp, void *priv)
{
    const char *name = priv;
    if (strcmp(tp->name, name) == 0)
        tp_ret = tp;
}

/* Tracepoint handler for sched_switch */
static void probe_sched_switch(void *data, bool preempt,
                             struct task_struct *prev,
                             struct task_struct *next,
                             unsigned int prev_state)
{
    struct trace_entry entry;
    
    if (!should_trace())
        return;
    
    /* Prepare trace entry */
    entry.timestamp = ktime_get_ns();
    entry.pid = next->pid;
    strncpy(entry.comm, next->comm, TASK_COMM_LEN);
    snprintf(entry.data, sizeof(entry.data),
             "Task switch: %s[%d] -> %s[%d] %s state=%u",
             prev->comm, prev->pid,
             next->comm, next->pid,
             preempt ? "(preempted)" : "(normal)",
             prev_state);
    
    /* Add to ring buffer */
    write_trace_entry(&entry);
}

/* Tracepoint handler for sched_wakeup */
static void probe_sched_wakeup(void *data, struct task_struct *task)
{
    struct trace_entry entry;
    
    if (!should_trace())
        return;
    
    /* Prepare trace entry */
    entry.timestamp = ktime_get_ns();
    entry.pid = task->pid;
    strncpy(entry.comm, task->comm, TASK_COMM_LEN);
    snprintf(entry.data, sizeof(entry.data),
             "Task wakeup: %s[%d] by %s[%d]",
             task->comm, task->pid,
             current->comm, current->pid);
    
    /* Add to ring buffer */
    write_trace_entry(&entry);
}

/* Declare tracepoint probes */
static struct tracepoint *tp_sched_switch;
static struct tracepoint *tp_sched_wakeup;

/* Find tracepoint by name */
static struct tracepoint *find_tracepoint(const char *name)
{
    tp_ret = NULL;
    for_each_kernel_tracepoint(tracepoint_lookup, (void *)name);
    return tp_ret;
}

/* Initialize tracepoint tracing */
int init_tracepoint_trace(void)
{
    int ret;

    /* Find tracepoints */
    tp_sched_switch = find_tracepoint("sched_switch");
    if (!tp_sched_switch) {
        pr_err("Failed to find sched_switch tracepoint\n");
        return -ENOENT;
    }

    tp_sched_wakeup = find_tracepoint("sched_wakeup");
    if (!tp_sched_wakeup) {
        pr_err("Failed to find sched_wakeup tracepoint\n");
        return -ENOENT;
    }
    
    /* Register sched_switch tracepoint */
    ret = tracepoint_probe_register(tp_sched_switch,
                                  (void *)probe_sched_switch,
                                  NULL);
    if (ret) {
        pr_err("Failed to register sched_switch tracepoint\n");
        return ret;
    }
    
    /* Register sched_wakeup tracepoint */
    ret = tracepoint_probe_register(tp_sched_wakeup,
                                  (void *)probe_sched_wakeup,
                                  NULL);
    if (ret) {
        pr_err("Failed to register sched_wakeup tracepoint\n");
        tracepoint_probe_unregister(tp_sched_switch,
                                  (void *)probe_sched_switch,
                                  NULL);
        return ret;
    }
    
    pr_info("Registered scheduler tracepoints\n");
    return 0;
}

/* Cleanup tracepoint tracing */
void cleanup_tracepoint_trace(void)
{
    if (tp_sched_wakeup)
        tracepoint_probe_unregister(tp_sched_wakeup,
                                  (void *)probe_sched_wakeup,
                                  NULL);
    if (tp_sched_switch)
        tracepoint_probe_unregister(tp_sched_switch,
                                  (void *)probe_sched_switch,
                                  NULL);
    pr_info("Unregistered scheduler tracepoints\n");
} 