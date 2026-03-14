#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PerCPU Demo");
MODULE_DESCRIPTION("Demonstration of Linux kernel per-CPU variables");

/* Static per-CPU variable declaration */
static DEFINE_PER_CPU(long, static_counter);
static DEFINE_PER_CPU(struct {
    long count;
    unsigned long timestamp;
    char data[32];
}, static_complex);

/* Dynamic per-CPU variable */
static long __percpu *dynamic_counter;
static struct dyn_complex __percpu {
    long count;
    unsigned long timestamp;
    spinlock_t lock;
    struct {
        int nested_count;
        char nested_data[16];
    } nested;
} *dynamic_complex;

/* Per-CPU array */
static long __percpu *percpu_array[10];

/* Statistics */
static atomic_t total_updates = ATOMIC_INIT(0);
static atomic_t total_reads = ATOMIC_INIT(0);

/* Thread control */
static struct task_struct *update_thread;
static struct task_struct *read_thread;
static bool should_stop;

/* Procfs entries */
static struct proc_dir_entry *demo_dir;
static struct proc_dir_entry *stats_entry;
static struct proc_dir_entry *control_entry;

/* Helper function to demonstrate remote per-CPU access */
static void update_remote_cpu(int cpu)
{
    unsigned long flags;
    struct dyn_complex __percpu *complex = dynamic_complex;
    struct dyn_complex *cpu_complex;
    
    /* Update static counter on remote CPU */
    per_cpu(static_counter, cpu)++;
    
    /* Update dynamic counter on remote CPU */
    (*per_cpu_ptr(dynamic_counter, cpu))++;
    
    /* Update complex structure on remote CPU */
    cpu_complex = per_cpu_ptr(complex, cpu);
    spin_lock_irqsave(&cpu_complex->lock, flags);
    cpu_complex->count++;
    cpu_complex->timestamp = jiffies;
    cpu_complex->nested.nested_count++;
    snprintf(cpu_complex->nested.nested_data, 
             sizeof(cpu_complex->nested.nested_data),
             "CPU%d", cpu);
    spin_unlock_irqrestore(&cpu_complex->lock, flags);
    
    atomic_inc(&total_updates);
}

/* Thread to continuously update per-CPU variables */
static int update_thread_fn(void *data)
{
    unsigned int cpu;
    struct dyn_complex *complex;
    unsigned long flags;
    
    while (!kthread_should_stop() && !should_stop) {
        /* Update static per-CPU counter for current CPU */
        this_cpu_inc(static_counter);
        
        /* Update dynamic per-CPU counter */
        this_cpu_inc(*dynamic_counter);
        
        /* Update complex structure */
        complex = this_cpu_ptr(dynamic_complex);
        spin_lock_irqsave(&complex->lock, flags);
        complex->count++;
        complex->timestamp = jiffies;
        complex->nested.nested_count++;
        snprintf(complex->nested.nested_data, 
                 sizeof(complex->nested.nested_data),
                 "CPU%d", smp_processor_id());
        spin_unlock_irqrestore(&complex->lock, flags);
        
        /* Update per-CPU array */
        for (cpu = 0; cpu < 10; cpu++)
            this_cpu_inc(*percpu_array[cpu]);
        
        /* Update remote CPUs */
        for_each_online_cpu(cpu)
            if (cpu != smp_processor_id())
                update_remote_cpu(cpu);
        
        atomic_inc(&total_updates);
        msleep_interruptible(1000);
    }
    
    return 0;
}

/* Thread to read per-CPU variables */
static int read_thread_fn(void *data)
{
    unsigned int cpu;
    struct dyn_complex *complex;
    unsigned long flags;
    
    while (!kthread_should_stop() && !should_stop) {
        /* Read all per-CPU variables */
        for_each_online_cpu(cpu) {
            /* Read static counter */
            pr_info("CPU%u static counter: %ld\n",
                    cpu, per_cpu(static_counter, cpu));
            
            /* Read dynamic counter */
            pr_info("CPU%u dynamic counter: %ld\n",
                    cpu, *per_cpu_ptr(dynamic_counter, cpu));
            
            /* Read complex structure */
            complex = per_cpu_ptr(dynamic_complex, cpu);
            spin_lock_irqsave(&complex->lock, flags);
            pr_info("CPU%u complex: count=%ld, timestamp=%lu, nested_count=%d, nested_data=%s\n",
                    cpu, complex->count, complex->timestamp,
                    complex->nested.nested_count, complex->nested.nested_data);
            spin_unlock_irqrestore(&complex->lock, flags);
            
            /* Read per-CPU array */
            pr_info("CPU%u array[0]: %ld\n",
                    cpu, *per_cpu_ptr(percpu_array[0], cpu));
        }
        
        atomic_inc(&total_reads);
        msleep_interruptible(5000);
    }
    
    return 0;
}

/* Procfs handlers */
static ssize_t stats_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[512];
    int len;
    unsigned int cpu;
    
    len = snprintf(buf, sizeof(buf),
                  "Per-CPU Statistics:\n"
                  "Total updates: %d\n"
                  "Total reads: %d\n"
                  "\nPer-CPU Counters:\n",
                  atomic_read(&total_updates),
                  atomic_read(&total_reads));
    
    for_each_online_cpu(cpu) {
        len += snprintf(buf + len, sizeof(buf) - len,
                       "CPU%u: static=%ld, dynamic=%ld\n",
                       cpu,
                       per_cpu(static_counter, cpu),
                       *per_cpu_ptr(dynamic_counter, cpu));
    }
    
    if (*ppos >= len)
        return 0;
    
    if (count > len - *ppos)
        count = len - *ppos;
    
    if (copy_to_user(ubuf, buf + *ppos, count))
        return -EFAULT;
    
    *ppos += count;
    return count;
}

static ssize_t control_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[32];
    size_t buf_size;
    unsigned int cpu;
    
    buf_size = min(count, sizeof(buf) - 1);
    if (copy_from_user(buf, ubuf, buf_size))
        return -EFAULT;
    
    buf[buf_size] = '\0';
    
    if (strncmp(buf, "reset", 5) == 0) {
        /* Reset all counters */
        for_each_online_cpu(cpu) {
            per_cpu(static_counter, cpu) = 0;
            *per_cpu_ptr(dynamic_counter, cpu) = 0;
            
            for (int i = 0; i < 10; i++)
                *per_cpu_ptr(percpu_array[i], cpu) = 0;
        }
        atomic_set(&total_updates, 0);
        atomic_set(&total_reads, 0);
        pr_info("All counters reset\n");
    }
    
    return count;
}

/* Procfs file operations */
static const struct proc_ops stats_ops = {
    .proc_read = stats_read,
};

static const struct proc_ops control_ops = {
    .proc_write = control_write,
};

static int __init percpu_demo_init(void)
{
    int cpu, i, ret = -ENOMEM;
    struct dyn_complex *complex;
    
    /* Initialize static per-CPU variables */
    for_each_online_cpu(cpu) {
        per_cpu(static_counter, cpu) = 0;
        per_cpu(static_complex, cpu).count = 0;
        per_cpu(static_complex, cpu).timestamp = jiffies;
    }
    
    /* Allocate dynamic per-CPU counter */
    dynamic_counter = alloc_percpu(long);
    if (!dynamic_counter)
        goto err_counter;
    
    /* Allocate dynamic per-CPU complex structure */
    dynamic_complex = alloc_percpu(struct dyn_complex);
    if (!dynamic_complex)
        goto err_complex;
    
    /* Initialize dynamic complex structure */
    for_each_online_cpu(cpu) {
        complex = per_cpu_ptr(dynamic_complex, cpu);
        complex->count = 0;
        complex->timestamp = jiffies;
        spin_lock_init(&complex->lock);
        complex->nested.nested_count = 0;
        memset(complex->nested.nested_data, 0, sizeof(complex->nested.nested_data));
    }
    
    /* Allocate per-CPU array */
    for (i = 0; i < 10; i++) {
        percpu_array[i] = alloc_percpu(long);
        if (!percpu_array[i])
            goto err_array;
        
        for_each_online_cpu(cpu)
            *per_cpu_ptr(percpu_array[i], cpu) = 0;
    }
    
    /* Create procfs entries */
    demo_dir = proc_mkdir("percpu_demo", NULL);
    if (!demo_dir)
        goto err_procfs;
    
    stats_entry = proc_create("stats", 0444, demo_dir, &stats_ops);
    if (!stats_entry)
        goto err_stats;
    
    control_entry = proc_create("control", 0220, demo_dir, &control_ops);
    if (!control_entry)
        goto err_control;
    
    /* Start threads */
    update_thread = kthread_run(update_thread_fn, NULL, "percpu_update");
    if (IS_ERR(update_thread))
        goto err_update_thread;
    
    read_thread = kthread_run(read_thread_fn, NULL, "percpu_read");
    if (IS_ERR(read_thread))
        goto err_read_thread;
    
    pr_info("Per-CPU demo module loaded\n");
    return 0;

err_read_thread:
    kthread_stop(update_thread);
err_update_thread:
    proc_remove(control_entry);
err_control:
    proc_remove(stats_entry);
err_stats:
    proc_remove(demo_dir);
err_procfs:
    for (i = 0; i < 10; i++)
        if (percpu_array[i])
            free_percpu(percpu_array[i]);
err_array:
    free_percpu(dynamic_complex);
err_complex:
    free_percpu(dynamic_counter);
err_counter:
    return ret;
}

static void __exit percpu_demo_exit(void)
{
    int i;
    
    /* Stop threads */
    should_stop = true;
    kthread_stop(read_thread);
    kthread_stop(update_thread);
    
    /* Remove procfs entries */
    proc_remove(control_entry);
    proc_remove(stats_entry);
    proc_remove(demo_dir);
    
    /* Free per-CPU variables */
    for (i = 0; i < 10; i++)
        free_percpu(percpu_array[i]);
    free_percpu(dynamic_complex);
    free_percpu(dynamic_counter);
    
    pr_info("Per-CPU demo module unloaded\n");
}

module_init(percpu_demo_init);
module_exit(percpu_demo_exit); 