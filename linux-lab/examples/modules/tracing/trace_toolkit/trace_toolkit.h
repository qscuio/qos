#ifndef _TRACE_TOOLKIT_H
#define _TRACE_TOOLKIT_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/ftrace.h>
#include <linux/tracepoint.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/ring_buffer.h>
#include <linux/kallsyms.h>

#define TRACE_TOOLKIT_NAME "trace_toolkit"
#define MAX_FILTER_LEN 256
#define MAX_TRACE_ENTRIES 1024

/* Kallsyms lookup type definition */
typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
extern kallsyms_lookup_name_t kallsyms_lookup_name_func;
extern unsigned long get_kallsyms_func(const char *name);

/* Common trace entry structure */
struct trace_entry {
    u64 timestamp;
    pid_t pid;
    char comm[TASK_COMM_LEN];
    char data[128];
};

/* Filter configuration */
struct trace_filter {
    char function_name[MAX_FILTER_LEN];
    pid_t target_pid;
    bool enabled;
};

/* External function declarations */
extern int init_kprobe_trace(void);
extern void cleanup_kprobe_trace(void);
extern int init_ftrace_trace(void);
extern void cleanup_ftrace_trace(void);
extern int init_tracepoint_trace(void);
extern void cleanup_tracepoint_trace(void);

/* Common functions */
extern struct dentry *trace_toolkit_dir;
extern struct trace_filter current_filter;
extern struct trace_buffer *trace_buffer;

static inline bool should_trace(void)
{
    if (!current_filter.enabled)
        return true;
    
    if (current_filter.target_pid && current_filter.target_pid != current->pid)
        return false;
    
    return true;
}

/* Helper function for writing to ring buffer */
static inline void write_trace_entry(struct trace_entry *entry)
{
    unsigned long flags;
    local_irq_save(flags);
    ring_buffer_write(trace_buffer, sizeof(struct trace_entry), (void *)entry);
    local_irq_restore(flags);
}

#endif /* _TRACE_TOOLKIT_H */ 