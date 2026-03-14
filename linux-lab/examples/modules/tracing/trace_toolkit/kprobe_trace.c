#include "trace_toolkit.h"

/* Kprobe handler for do_sys_open */
static struct kprobe kp = {
    .symbol_name = "do_sys_open",
};

/* Pre-handler: called before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct trace_entry entry;
    char *filename;
    
    if (!should_trace())
        return 0;
    
    /* Get filename from registers */
#ifdef CONFIG_X86_64
    filename = (char *)regs->di;
#else
    filename = (char *)regs->ax;
#endif
    
    /* Prepare trace entry */
    entry.timestamp = ktime_get_ns();
    entry.pid = current->pid;
    strncpy(entry.comm, current->comm, TASK_COMM_LEN);
    snprintf(entry.data, sizeof(entry.data), "Opening file: %s", 
             filename ? filename : "unknown");
    
    /* Add to ring buffer */
    write_trace_entry(&entry);
    
    return 0;
}

/* Post-handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
                        unsigned long flags)
{
    struct trace_entry entry;
    long ret;
    
    if (!should_trace())
        return;
    
    /* Get return value */
    ret = regs_return_value(regs);
    
    /* Prepare trace entry */
    entry.timestamp = ktime_get_ns();
    entry.pid = current->pid;
    strncpy(entry.comm, current->comm, TASK_COMM_LEN);
    snprintf(entry.data, sizeof(entry.data), "File open returned: %ld", ret);
    
    /* Add to ring buffer */
    write_trace_entry(&entry);
}

/* Initialize kprobe tracing */
int init_kprobe_trace(void)
{
    int ret;
    
    /* Register kprobe */
    kp.pre_handler = handler_pre;
    kp.post_handler = handler_post;
    
    ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("register_kprobe failed, returned %d\n", ret);
        return ret;
    }
    
    pr_info("Planted kprobe at %s\n", kp.symbol_name);
    return 0;
}

/* Cleanup kprobe tracing */
void cleanup_kprobe_trace(void)
{
    unregister_kprobe(&kp);
    pr_info("Removed kprobe at %s\n", kp.symbol_name);
} 