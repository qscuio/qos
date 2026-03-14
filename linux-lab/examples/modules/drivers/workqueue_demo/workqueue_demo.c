#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/cpu.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Workqueue Demo");
MODULE_DESCRIPTION("Advanced demonstration of Linux kernel workqueues");

/* Workqueue types demonstration */
static struct workqueue_struct *demo_wq;         /* Custom workqueue */
static struct workqueue_struct *demo_highpri_wq; /* High priority workqueue */
static struct workqueue_struct **demo_percpu_wq; /* Per-CPU workqueues */

/* Work structures */
static struct work_struct immediate_work;
static struct delayed_work delayed_work;
static struct work_struct ordered_works[3];      /* For ordered execution demo */
static struct delayed_work requeue_work;         /* Self-requeuing work */
static DECLARE_COMPLETION(dynamic_works_done);   /* For dynamic work completion */

/* Per-CPU work structures */
static DEFINE_PER_CPU(struct work_struct, percpu_work);

/* Work item pool management */
static struct kmem_cache *work_item_cache;      /* Slab cache for work items */
#define MAX_DYNAMIC_WORKS 10
static atomic_t dynamic_work_count = ATOMIC_INIT(0);

/* Statistics */
static atomic_t immediate_count = ATOMIC_INIT(0);
static atomic_t delayed_count = ATOMIC_INIT(0);
static atomic_t dynamic_count = ATOMIC_INIT(0);
static atomic_t ordered_count = ATOMIC_INIT(0);
static atomic_t percpu_count = ATOMIC_INIT(0);
static atomic_t requeue_count = ATOMIC_INIT(0);

/* Configuration */
static struct {
    unsigned int dynamic_work_delay_ms;
    unsigned int requeue_interval_ms;
    bool cpu_intensive;
    unsigned int target_cpu;
} demo_config = {
    .dynamic_work_delay_ms = 1000,
    .requeue_interval_ms = 5000,
    .cpu_intensive = false,
    .target_cpu = 0
};

/* Procfs entries */
static struct proc_dir_entry *demo_dir;
static struct proc_dir_entry *trigger_entry;
static struct proc_dir_entry *stats_entry;
static struct proc_dir_entry *config_entry;

/* Work handler helpers */
static void cpu_intensive_work(void)
{
    unsigned int i, j;
    volatile unsigned long result = 0;
    
    if (!demo_config.cpu_intensive)
        return;
        
    /* Simulate CPU-intensive work */
    for (i = 0; i < 1000000; i++)
        for (j = 0; j < 100; j++)
            result += i * j;
}

/* Dynamic work item structure */
struct dynamic_work_data {
    struct work_struct work;
    unsigned int id;
    unsigned long timestamp;
    struct list_head list;
};

/* Work Handlers */
static void immediate_work_handler(struct work_struct *work)
{
    unsigned long flags;
    
    local_irq_save(flags);
    pr_info("Executing immediate work on CPU %d\n", smp_processor_id());
    local_irq_restore(flags);
    
    cpu_intensive_work();
    atomic_inc(&immediate_count);
}

static void delayed_work_handler(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    unsigned long delay = msecs_to_jiffies(demo_config.dynamic_work_delay_ms);
    
    pr_info("Executing delayed work after %u ms\n", demo_config.dynamic_work_delay_ms);
    atomic_inc(&delayed_count);
    cpu_intensive_work();
    
    /* Demonstrate work rescheduling */
    schedule_delayed_work(dwork, delay);
}

static void ordered_work_handler(struct work_struct *work)
{
    int id = work - ordered_works;
    unsigned long flags;
    
    local_irq_save(flags);
    pr_info("Executing ordered work %d on CPU %d\n", id, smp_processor_id());
    local_irq_restore(flags);
    
    cpu_intensive_work();
    msleep(1000 * (3 - id)); /* Reverse sleep to test ordering */
    atomic_inc(&ordered_count);
}

static void dynamic_work_handler(struct work_struct *work)
{
    struct dynamic_work_data *data = container_of(work, struct dynamic_work_data, work);
    unsigned long runtime;
    
    runtime = jiffies - data->timestamp;
    pr_info("Dynamic work %u executed after %u ms on CPU %d\n",
            data->id, jiffies_to_msecs(runtime), smp_processor_id());
    
    atomic_inc(&dynamic_count);
    cpu_intensive_work();
    
    if (atomic_dec_and_test(&dynamic_work_count))
        complete(&dynamic_works_done);
        
    kmem_cache_free(work_item_cache, data);
}

static void percpu_work_handler(struct work_struct *work)
{
    unsigned int cpu = smp_processor_id();
    
    pr_info("Per-CPU work executing on CPU %u\n", cpu);
    cpu_intensive_work();
    atomic_inc(&percpu_count);
}

static void requeue_work_handler(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    
    pr_info("Requeue work handler (count: %d)\n", atomic_read(&requeue_count));
    atomic_inc(&requeue_count);
    
    /* Requeue itself unless count reaches limit */
    if (atomic_read(&requeue_count) < 10)
        schedule_delayed_work(dwork, msecs_to_jiffies(demo_config.requeue_interval_ms));
}

/* Procfs handlers */
static ssize_t stats_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[512];
    int len;
    
    len = snprintf(buf, sizeof(buf),
                  "Workqueue Statistics:\n"
                  "  Immediate work executed: %d times\n"
                  "  Delayed work executed:   %d times\n"
                  "  Dynamic work executed:   %d times\n"
                  "  Per-CPU work executed:   %d times\n"
                  "  Requeue work executed:   %d times\n"
                  "\nConfiguration:\n"
                  "  Dynamic work delay: %u ms\n"
                  "  Requeue interval:   %u ms\n"
                  "  CPU intensive:      %s\n"
                  "  Target CPU:         %u\n",
                  atomic_read(&immediate_count),
                  atomic_read(&delayed_count),
                  atomic_read(&dynamic_count),
                  atomic_read(&percpu_count),
                  atomic_read(&requeue_count),
                  demo_config.dynamic_work_delay_ms,
                  demo_config.requeue_interval_ms,
                  demo_config.cpu_intensive ? "yes" : "no",
                  demo_config.target_cpu);
    
    if (*ppos >= len)
        return 0;
    
    if (count > len - *ppos)
        count = len - *ppos;
    
    if (copy_to_user(ubuf, buf + *ppos, count))
        return -EFAULT;
    
    *ppos += count;
    return count;
}

static ssize_t trigger_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[32];
    size_t buf_size;
    int i;
    struct dynamic_work_data *data;
    
    buf_size = min(count, sizeof(buf) - 1);
    if (copy_from_user(buf, ubuf, buf_size))
        return -EFAULT;
    
    buf[buf_size] = '\0';
    
    if (strncmp(buf, "immediate", 9) == 0) {
        queue_work_on(demo_config.target_cpu, demo_wq, &immediate_work);
    }
    else if (strncmp(buf, "delayed", 7) == 0) {
        schedule_delayed_work(&delayed_work, 
                            msecs_to_jiffies(demo_config.dynamic_work_delay_ms));
    }
    else if (strncmp(buf, "ordered", 7) == 0) {
        /* Queue works in sequence */
        for (i = 0; i < 3; i++)
            queue_work(demo_wq, &ordered_works[i]);
    }
    else if (strncmp(buf, "dynamic", 7) == 0) {
        /* Reset completion and counter */
        reinit_completion(&dynamic_works_done);
        atomic_set(&dynamic_work_count, MAX_DYNAMIC_WORKS);
        
        /* Create and queue dynamic works */
        for (i = 0; i < MAX_DYNAMIC_WORKS; i++) {
            data = kmem_cache_alloc(work_item_cache, GFP_KERNEL);
            if (!data) {
                pr_err("Failed to allocate dynamic work data\n");
                continue;
            }
            
            INIT_WORK(&data->work, dynamic_work_handler);
            data->id = i;
            data->timestamp = jiffies;
            
            queue_work_on(demo_config.target_cpu, demo_wq, &data->work);
        }
        
        /* Wait for all dynamic works to complete */
        wait_for_completion(&dynamic_works_done);
        pr_info("All dynamic works completed\n");
    }
    else if (strncmp(buf, "percpu", 6) == 0) {
        /* Queue work on each CPU */
        for_each_online_cpu(i) {
            struct work_struct *work = per_cpu_ptr(&percpu_work, i);
            queue_work_on(i, demo_percpu_wq[i], work);
        }
    }
    else if (strncmp(buf, "requeue", 7) == 0) {
        schedule_delayed_work(&requeue_work, 
                            msecs_to_jiffies(demo_config.requeue_interval_ms));
    }
    else if (strncmp(buf, "cancel", 6) == 0) {
        cancel_delayed_work_sync(&delayed_work);
        cancel_delayed_work_sync(&requeue_work);
        pr_info("Cancelled all delayed works\n");
    }
    else if (strncmp(buf, "flush", 5) == 0) {
        flush_workqueue(demo_wq);
        flush_workqueue(demo_highpri_wq);
        for_each_online_cpu(i)
            if (demo_percpu_wq[i])
                flush_workqueue(demo_percpu_wq[i]);
        pr_info("Flushed all workqueues\n");
    }
    
    return count;
}

static ssize_t config_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[32];
    size_t buf_size;
    unsigned int value;
    
    buf_size = min(count, sizeof(buf) - 1);
    if (copy_from_user(buf, ubuf, buf_size))
        return -EFAULT;
    
    buf[buf_size] = '\0';
    
    if (sscanf(buf, "delay %u", &value) == 1) {
        demo_config.dynamic_work_delay_ms = value;
    }
    else if (sscanf(buf, "interval %u", &value) == 1) {
        demo_config.requeue_interval_ms = value;
    }
    else if (sscanf(buf, "cpu %u", &value) == 1) {
        if (value < num_online_cpus())
            demo_config.target_cpu = value;
    }
    else if (strncmp(buf, "cpu_intensive", 12) == 0) {
        demo_config.cpu_intensive = !demo_config.cpu_intensive;
    }
    
    return count;
}

/* Procfs file operations */
static const struct proc_ops stats_ops = {
    .proc_read = stats_read,
};

static const struct proc_ops trigger_ops = {
    .proc_write = trigger_write,
};

static const struct proc_ops config_ops = {
    .proc_write = config_write,
};

static int __init workqueue_demo_init(void)
{
    int cpu, ret = -ENOMEM;
    
    /* Create slab cache for dynamic work items */
    work_item_cache = kmem_cache_create("work_item_cache",
                                      sizeof(struct dynamic_work_data),
                                      0, SLAB_HWCACHE_ALIGN, NULL);
    if (!work_item_cache)
        goto err_cache;
    
    /* Create various types of workqueues */
    demo_wq = alloc_workqueue("demo_wq", WQ_UNBOUND | WQ_HIGHPRI | 
                             WQ_CPU_INTENSIVE, num_online_cpus());
    if (!demo_wq)
        goto err_wq;
    
    demo_highpri_wq = alloc_workqueue("demo_highpri",
                                    WQ_HIGHPRI | WQ_CPU_INTENSIVE, 1);
    if (!demo_highpri_wq)
        goto err_highpri_wq;
    
    /* Allocate per-CPU workqueue array */
    demo_percpu_wq = kcalloc(num_online_cpus(), 
                            sizeof(struct workqueue_struct *), GFP_KERNEL);
    if (!demo_percpu_wq)
        goto err_percpu_alloc;
    
    /* Create per-CPU workqueues */
    for_each_online_cpu(cpu) {
        char name[32];
        snprintf(name, sizeof(name), "demo_cpu%d", cpu);
        
        demo_percpu_wq[cpu] = alloc_workqueue(name, WQ_CPU_INTENSIVE, 1);
        if (!demo_percpu_wq[cpu]) {
            pr_err("Failed to create per-CPU workqueue for CPU %d\n", cpu);
            goto err_percpu_wq;
        }
    }
    
    /* Initialize work structures */
    INIT_WORK(&immediate_work, immediate_work_handler);
    INIT_DELAYED_WORK(&delayed_work, delayed_work_handler);
    INIT_DELAYED_WORK(&requeue_work, requeue_work_handler);
    
    for (cpu = 0; cpu < 3; cpu++)
        INIT_WORK(&ordered_works[cpu], ordered_work_handler);
    
    for_each_online_cpu(cpu) {
        struct work_struct *work = per_cpu_ptr(&percpu_work, cpu);
        INIT_WORK(work, percpu_work_handler);
    }
    
    /* Create procfs entries */
    demo_dir = proc_mkdir("workqueue_demo", NULL);
    if (!demo_dir)
        goto err_procfs;
    
    trigger_entry = proc_create("trigger", 0220, demo_dir, &trigger_ops);
    if (!trigger_entry)
        goto err_trigger;
    
    stats_entry = proc_create("stats", 0444, demo_dir, &stats_ops);
    if (!stats_entry)
        goto err_stats;
    
    config_entry = proc_create("config", 0220, demo_dir, &config_ops);
    if (!config_entry)
        goto err_config;
    
    pr_info("Advanced workqueue demo module loaded\n");
    return 0;

err_config:
    proc_remove(stats_entry);
err_stats:
    proc_remove(trigger_entry);
err_trigger:
    proc_remove(demo_dir);
err_procfs:
    for_each_online_cpu(cpu) {
        if (demo_percpu_wq[cpu])
            destroy_workqueue(demo_percpu_wq[cpu]);
    }
err_percpu_wq:
    kfree(demo_percpu_wq);
err_percpu_alloc:
    destroy_workqueue(demo_highpri_wq);
err_highpri_wq:
    destroy_workqueue(demo_wq);
err_wq:
    kmem_cache_destroy(work_item_cache);
err_cache:
    return ret;
}

static void __exit workqueue_demo_exit(void)
{
    int cpu;
    
    /* Cancel any pending work */
    cancel_delayed_work_sync(&delayed_work);
    cancel_delayed_work_sync(&requeue_work);
    
    /* Ensure all work items are completed */
    flush_workqueue(demo_wq);
    flush_workqueue(demo_highpri_wq);
    
    for_each_online_cpu(cpu)
        if (demo_percpu_wq[cpu])
            flush_workqueue(demo_percpu_wq[cpu]);
    
    /* Clean up procfs entries */
    proc_remove(config_entry);
    proc_remove(stats_entry);
    proc_remove(trigger_entry);
    proc_remove(demo_dir);
    
    /* Destroy workqueues */
    for_each_online_cpu(cpu)
        if (demo_percpu_wq[cpu])
            destroy_workqueue(demo_percpu_wq[cpu]);
    
    kfree(demo_percpu_wq);
    destroy_workqueue(demo_highpri_wq);
    destroy_workqueue(demo_wq);
    
    /* Destroy slab cache */
    kmem_cache_destroy(work_item_cache);
    
    pr_info("Advanced workqueue demo module unloaded\n");
}

module_init(workqueue_demo_init);
module_exit(workqueue_demo_exit); 