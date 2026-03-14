#include "trace_toolkit.h"

/* Function to trace */
static unsigned long target_func_addr;

/* ftrace_ops structure */
static struct ftrace_ops trace_ops;

/* Callback function for ftrace */
static void trace_function(unsigned long ip, unsigned long parent_ip,
                          struct ftrace_ops *op, struct ftrace_regs *regs)
{
    struct trace_entry entry;
    
    if (!should_trace())
        return;
    
    /* Prepare trace entry */
    entry.timestamp = ktime_get_ns();
    entry.pid = current->pid;
    strncpy(entry.comm, current->comm, TASK_COMM_LEN);
    snprintf(entry.data, sizeof(entry.data),
             "Function called: ip=0x%lx parent_ip=0x%lx",
             ip, parent_ip);
    
    /* Add to ring buffer */
    write_trace_entry(&entry);
}

/* Initialize ftrace */
int init_ftrace_trace(void)
{
    int ret;
    
    /* Get address of function to trace (e.g., schedule) */
    target_func_addr = get_kallsyms_func("schedule");
    if (!target_func_addr) {
        pr_err("Failed to lookup schedule function\n");
        return -EINVAL;
    }
    
    /* Setup ftrace ops */
    trace_ops.func = trace_function;
    trace_ops.flags = FTRACE_OPS_FL_IPMODIFY | FTRACE_OPS_FL_SAVE_REGS;
    
    /* Register ftrace function */
    ret = ftrace_set_filter_ip(&trace_ops, target_func_addr, 0, 0);
    if (ret) {
        pr_err("ftrace_set_filter_ip failed: %d\n", ret);
        return ret;
    }
    
    ret = register_ftrace_function(&trace_ops);
    if (ret) {
        pr_err("register_ftrace_function failed: %d\n", ret);
        ftrace_set_filter_ip(&trace_ops, target_func_addr, 1, 0);
        return ret;
    }
    
    pr_info("Registered ftrace probe for schedule()\n");
    return 0;
}

/* Cleanup ftrace */
void cleanup_ftrace_trace(void)
{
    unregister_ftrace_function(&trace_ops);
    ftrace_set_filter_ip(&trace_ops, target_func_addr, 1, 0);
    pr_info("Unregistered ftrace probe\n");
} 