#include "trace_toolkit.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Trace Toolkit");
MODULE_DESCRIPTION("A comprehensive Linux kernel tracing toolkit");

/* Global variables */
struct dentry *trace_toolkit_dir;
struct trace_filter current_filter;
struct trace_buffer *trace_buffer;
static struct mutex trace_lock;

/* Kallsyms lookup functionality */
kallsyms_lookup_name_t kallsyms_lookup_name_func;

static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};

/* Get kallsyms_lookup_name address using kprobe */
static int init_kallsyms_lookup(void)
{
    int ret;

    ret = register_kprobe(&kp);
    if (ret < 0)
        return ret;

    kallsyms_lookup_name_func = (kallsyms_lookup_name_t)kp.addr;
    unregister_kprobe(&kp);

    return 0;
}

/* Exported kallsyms lookup function */
unsigned long get_kallsyms_func(const char *name)
{
    if (!kallsyms_lookup_name_func)
        return 0;
    return kallsyms_lookup_name_func(name);
}
EXPORT_SYMBOL_GPL(get_kallsyms_func);

/* Filter control file operations */
static ssize_t filter_write(struct file *file, const char __user *user_buf,
                          size_t count, loff_t *ppos)
{
    char buf[MAX_FILTER_LEN];
    size_t buf_size;
    char *pid_str;
    
    buf_size = min(count, (size_t)(MAX_FILTER_LEN - 1));
    if (copy_from_user(buf, user_buf, buf_size))
        return -EFAULT;
    buf[buf_size] = '\0';
    
    mutex_lock(&trace_lock);
    
    if (buf[0] == '0') {
        /* Disable filtering */
        current_filter.enabled = false;
        current_filter.target_pid = 0;
        memset(current_filter.function_name, 0, MAX_FILTER_LEN);
    } else {
        /* Format: function_name:pid */
        current_filter.enabled = true;
        pid_str = strchr(buf, ':');
        if (pid_str) {
            *pid_str = '\0';
            pid_str++;
            if (kstrtoint(pid_str, 10, &current_filter.target_pid))
                current_filter.target_pid = 0;
        }
        strncpy(current_filter.function_name, buf, MAX_FILTER_LEN - 1);
    }
    
    mutex_unlock(&trace_lock);
    return count;
}

static ssize_t filter_read(struct file *file, char __user *user_buf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_FILTER_LEN * 2];
    int len;
    
    mutex_lock(&trace_lock);
    if (current_filter.enabled)
        len = snprintf(buf, sizeof(buf), "Filter: function=%s pid=%d\n",
                      current_filter.function_name,
                      current_filter.target_pid);
    else
        len = snprintf(buf, sizeof(buf), "Filtering disabled\n");
    mutex_unlock(&trace_lock);
    
    return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations filter_fops = {
    .owner = THIS_MODULE,
    .read = filter_read,
    .write = filter_write,
};

/* Trace data read file operations */
static ssize_t trace_read(struct file *file, char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    struct ring_buffer_event *event;
    struct trace_entry *entry;
    char *buf;
    size_t len = 0;
    size_t total_len = 0;
    u64 ts;
    unsigned long flags;
    
    buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;
    
    mutex_lock(&trace_lock);
    
    while ((event = ring_buffer_consume(trace_buffer, 0, &ts, &flags))) {
        entry = (struct trace_entry *)event;
        len = snprintf(buf + total_len, PAGE_SIZE - total_len,
                      "[%llu] %s[%d]: %s\n",
                      entry->timestamp, entry->comm,
                      entry->pid, entry->data);
        total_len += len;
        if (total_len >= PAGE_SIZE - 256)
            break;
    }
    
    mutex_unlock(&trace_lock);
    
    len = simple_read_from_buffer(user_buf, count, ppos, buf, total_len);
    kfree(buf);
    return len;
}

static const struct file_operations trace_fops = {
    .owner = THIS_MODULE,
    .read = trace_read,
};

static int __init trace_toolkit_init(void)
{
    int ret;
    
    /* Initialize kallsyms lookup */
    ret = init_kallsyms_lookup();
    if (ret < 0) {
        pr_err("Failed to initialize kallsyms lookup\n");
        return ret;
    }
    
    /* Initialize mutex */
    mutex_init(&trace_lock);
    
    /* Create debugfs directory */
    trace_toolkit_dir = debugfs_create_dir(TRACE_TOOLKIT_NAME, NULL);
    if (!trace_toolkit_dir) {
        pr_err("Failed to create debugfs directory\n");
        return -ENOMEM;
    }
    
    /* Create debugfs files */
    if (!debugfs_create_file("filter", 0644, trace_toolkit_dir, NULL, &filter_fops)) {
        pr_err("Failed to create filter file\n");
        ret = -ENOMEM;
        goto err_remove_dir;
    }
    
    if (!debugfs_create_file("trace", 0444, trace_toolkit_dir, NULL, &trace_fops)) {
        pr_err("Failed to create trace file\n");
        ret = -ENOMEM;
        goto err_remove_dir;
    }
    
    /* Initialize ring buffer */
    trace_buffer = ring_buffer_alloc(MAX_TRACE_ENTRIES * sizeof(struct trace_entry),
                                   RING_BUFFER_ALL_CPUS);
    if (!trace_buffer) {
        pr_err("Failed to allocate ring buffer\n");
        ret = -ENOMEM;
        goto err_remove_dir;
    }
    
    /* Initialize tracing components */
    ret = init_kprobe_trace();
    if (ret)
        goto err_free_buffer;
    
    ret = init_ftrace_trace();
    if (ret)
        goto err_cleanup_kprobe;
    
    ret = init_tracepoint_trace();
    if (ret)
        goto err_cleanup_ftrace;
    
    pr_info("Trace toolkit initialized\n");
    return 0;

err_cleanup_ftrace:
    cleanup_ftrace_trace();
err_cleanup_kprobe:
    cleanup_kprobe_trace();
err_free_buffer:
    ring_buffer_free(trace_buffer);
err_remove_dir:
    debugfs_remove_recursive(trace_toolkit_dir);
    return ret;
}

static void __exit trace_toolkit_exit(void)
{
    cleanup_tracepoint_trace();
    cleanup_ftrace_trace();
    cleanup_kprobe_trace();
    ring_buffer_free(trace_buffer);
    debugfs_remove_recursive(trace_toolkit_dir);
    pr_info("Trace toolkit unloaded\n");
}

module_init(trace_toolkit_init);
module_exit(trace_toolkit_exit); 