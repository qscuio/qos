#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wakeup Demo");
MODULE_DESCRIPTION("Demonstration of Linux kernel wakeup mechanisms");

/* Thread and wait queue declarations */
static struct task_struct *consumer_thread;
static struct task_struct *producer_thread;
static struct task_struct *event_thread;
static struct task_struct *timeout_thread;
static struct task_struct *exclusive_thread;

/* Wait queues */
static DECLARE_WAIT_QUEUE_HEAD(simple_wait);
static DECLARE_WAIT_QUEUE_HEAD(event_wait);
static DECLARE_WAIT_QUEUE_HEAD(timeout_wait);
static DECLARE_WAIT_QUEUE_HEAD(exclusive_wait);

/* Completion structure */
static DECLARE_COMPLETION(task_completion);
static DECLARE_COMPLETION(batch_completion);

/* Synchronization */
static DEFINE_SPINLOCK(event_lock);
static DEFINE_MUTEX(shared_mutex);

/* Shared data and state */
static atomic_t data_ready = ATOMIC_INIT(0);
static atomic_t event_count = ATOMIC_INIT(0);
static atomic_t batch_count = ATOMIC_INIT(0);
static bool event_occurred;
static bool should_stop;
static unsigned int timeout_ms = 1000;

/* Timer for periodic wakeups */
static struct timer_list wakeup_timer;

/* Statistics */
static atomic_t wakeup_count = ATOMIC_INIT(0);
static atomic_t timeout_count = ATOMIC_INIT(0);
static atomic_t event_processed = ATOMIC_INIT(0);
static atomic_t exclusive_count = ATOMIC_INIT(0);

/* Procfs entries */
static struct proc_dir_entry *demo_dir;
static struct proc_dir_entry *control_entry;
static struct proc_dir_entry *stats_entry;
static struct proc_dir_entry *config_entry;

/* Timer callback */
static void wakeup_timer_fn(struct timer_list *t)
{
    /* Signal event occurrence */
    spin_lock(&event_lock);
    event_occurred = true;
    spin_unlock(&event_lock);
    
    /* Wake up waiters */
    wake_up_interruptible(&event_wait);
    
    /* Requeue timer if not stopping */
    if (!should_stop)
        mod_timer(&wakeup_timer, jiffies + msecs_to_jiffies(timeout_ms));
}

/* Thread functions */
static int consumer_thread_fn(void *data)
{
    while (!kthread_should_stop()) {
        /* Wait for data to be ready */
        wait_event_interruptible(simple_wait,
                               atomic_read(&data_ready) > 0 || 
                               kthread_should_stop());
        
        if (kthread_should_stop())
            break;
        
        /* Process data */
        if (atomic_dec_if_positive(&data_ready) >= 0) {
            atomic_inc(&wakeup_count);
            pr_info("Consumer processed data\n");
        }
    }
    
    return 0;
}

static int producer_thread_fn(void *data)
{
    while (!kthread_should_stop()) {
        /* Produce data */
        atomic_inc(&data_ready);
        pr_info("Producer generated data\n");
        
        /* Wake up consumer */
        wake_up_interruptible(&simple_wait);
        
        /* Wait before next production */
        msleep_interruptible(1000);
    }
    
    return 0;
}

static int event_thread_fn(void *data)
{
    unsigned long flags;
    
    while (!kthread_should_stop()) {
        /* Wait for event with timeout */
        wait_event_interruptible_timeout(event_wait,
                                       event_occurred || kthread_should_stop(),
                                       msecs_to_jiffies(2000));
        
        spin_lock_irqsave(&event_lock, flags);
        if (event_occurred) {
            event_occurred = false;
            atomic_inc(&event_processed);
            pr_info("Event processed\n");
        }
        spin_unlock_irqrestore(&event_lock, flags);
    }
    
    return 0;
}

static int timeout_thread_fn(void *data)
{
    int ret;
    
    while (!kthread_should_stop()) {
        /* Wait with timeout */
        ret = wait_event_interruptible_timeout(timeout_wait,
                                             kthread_should_stop(),
                                             msecs_to_jiffies(timeout_ms));
        
        if (ret == 0) {  /* Timeout occurred */
            atomic_inc(&timeout_count);
            pr_info("Timeout occurred\n");
        }
    }
    
    return 0;
}

static int exclusive_thread_fn(void *data)
{
    while (!kthread_should_stop()) {
        /* Exclusive wait */
        wait_event_interruptible_exclusive(exclusive_wait,
                                         atomic_read(&event_count) > 0 ||
                                         kthread_should_stop());
        
        if (kthread_should_stop())
            break;
        
        /* Process event exclusively */
        if (atomic_dec_if_positive(&event_count) >= 0) {
            mutex_lock(&shared_mutex);
            atomic_inc(&exclusive_count);
            pr_info("Exclusive event processed\n");
            mutex_unlock(&shared_mutex);
        }
    }
    
    return 0;
}

/* Batch processing demonstration */
static void process_batch(void)
{
    int i, count = 5;
    struct task_struct *batch_threads[5];
    
    /* Reset completion */
    reinit_completion(&batch_completion);
    atomic_set(&batch_count, count);
    
    /* Create batch threads */
    for (i = 0; i < count; i++) {
        batch_threads[i] = kthread_run(
            (int (*)(void *))({
                int batch_worker(void *data)
                {
                    /* Simulate work */
                    msleep_interruptible(500);
                    
                    /* Signal completion */
                    if (atomic_dec_and_test(&batch_count))
                        complete(&batch_completion);
                    
                    return 0;
                }
                batch_worker;
            }), NULL, "batch_worker/%d", i);
        
        if (IS_ERR(batch_threads[i])) {
            pr_err("Failed to create batch thread %d\n", i);
            count = i;
            break;
        }
    }
    
    /* Wait for batch completion */
    wait_for_completion(&batch_completion);
    pr_info("Batch processing completed\n");
    
    /* Stop batch threads */
    for (i = 0; i < count; i++)
        kthread_stop(batch_threads[i]);
}

/* Procfs handlers */
static ssize_t stats_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[512];
    int len;
    
    len = snprintf(buf, sizeof(buf),
                  "Wakeup Statistics:\n"
                  "  Normal wakeups:     %d\n"
                  "  Timeout wakeups:    %d\n"
                  "  Events processed:   %d\n"
                  "  Exclusive wakeups:  %d\n"
                  "\nConfiguration:\n"
                  "  Timeout interval:   %u ms\n",
                  atomic_read(&wakeup_count),
                  atomic_read(&timeout_count),
                  atomic_read(&event_processed),
                  atomic_read(&exclusive_count),
                  timeout_ms);
    
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
    
    buf_size = min(count, sizeof(buf) - 1);
    if (copy_from_user(buf, ubuf, buf_size))
        return -EFAULT;
    
    buf[buf_size] = '\0';
    
    if (strncmp(buf, "wakeup", 6) == 0) {
        /* Trigger normal wakeup */
        atomic_inc(&data_ready);
        wake_up_interruptible(&simple_wait);
    }
    else if (strncmp(buf, "event", 5) == 0) {
        /* Trigger event */
        unsigned long flags;
        spin_lock_irqsave(&event_lock, flags);
        event_occurred = true;
        spin_unlock_irqrestore(&event_lock, flags);
        wake_up_interruptible(&event_wait);
    }
    else if (strncmp(buf, "exclusive", 9) == 0) {
        /* Trigger exclusive wakeup */
        atomic_inc(&event_count);
        wake_up_interruptible(&exclusive_wait);
    }
    else if (strncmp(buf, "batch", 5) == 0) {
        /* Start batch processing */
        process_batch();
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
    
    if (sscanf(buf, "timeout %u", &value) == 1) {
        timeout_ms = value;
        mod_timer(&wakeup_timer, jiffies + msecs_to_jiffies(timeout_ms));
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

static const struct proc_ops config_ops = {
    .proc_write = config_write,
};

static int __init wakeup_demo_init(void)
{
    /* Initialize timer */
    timer_setup(&wakeup_timer, wakeup_timer_fn, 0);
    mod_timer(&wakeup_timer, jiffies + msecs_to_jiffies(timeout_ms));
    
    /* Create kernel threads */
    consumer_thread = kthread_run(consumer_thread_fn, NULL, "wakeup_consumer");
    if (IS_ERR(consumer_thread))
        goto err_consumer;
    
    producer_thread = kthread_run(producer_thread_fn, NULL, "wakeup_producer");
    if (IS_ERR(producer_thread))
        goto err_producer;
    
    event_thread = kthread_run(event_thread_fn, NULL, "wakeup_event");
    if (IS_ERR(event_thread))
        goto err_event;
    
    timeout_thread = kthread_run(timeout_thread_fn, NULL, "wakeup_timeout");
    if (IS_ERR(timeout_thread))
        goto err_timeout;
    
    exclusive_thread = kthread_run(exclusive_thread_fn, NULL, "wakeup_exclusive");
    if (IS_ERR(exclusive_thread))
        goto err_exclusive;
    
    /* Create procfs entries */
    demo_dir = proc_mkdir("wakeup_demo", NULL);
    if (!demo_dir)
        goto err_procfs;
    
    control_entry = proc_create("control", 0220, demo_dir, &control_ops);
    if (!control_entry)
        goto err_control;
    
    stats_entry = proc_create("stats", 0444, demo_dir, &stats_ops);
    if (!stats_entry)
        goto err_stats;
    
    config_entry = proc_create("config", 0220, demo_dir, &config_ops);
    if (!config_entry)
        goto err_config;
    
    pr_info("Wakeup demo module loaded\n");
    return 0;

err_config:
    proc_remove(stats_entry);
err_stats:
    proc_remove(control_entry);
err_control:
    proc_remove(demo_dir);
err_procfs:
    kthread_stop(exclusive_thread);
err_exclusive:
    kthread_stop(timeout_thread);
err_timeout:
    kthread_stop(event_thread);
err_event:
    kthread_stop(producer_thread);
err_producer:
    kthread_stop(consumer_thread);
err_consumer:
    del_timer_sync(&wakeup_timer);
    return -ENOMEM;
}

static void __exit wakeup_demo_exit(void)
{
    /* Stop timer */
    del_timer_sync(&wakeup_timer);
    
    /* Signal threads to stop */
    should_stop = true;
    
    /* Wake up all waiters */
    wake_up_interruptible_all(&simple_wait);
    wake_up_interruptible_all(&event_wait);
    wake_up_interruptible_all(&timeout_wait);
    wake_up_interruptible_all(&exclusive_wait);
    
    /* Stop threads */
    kthread_stop(exclusive_thread);
    kthread_stop(timeout_thread);
    kthread_stop(event_thread);
    kthread_stop(producer_thread);
    kthread_stop(consumer_thread);
    
    /* Remove procfs entries */
    proc_remove(config_entry);
    proc_remove(stats_entry);
    proc_remove(control_entry);
    proc_remove(demo_dir);
    
    pr_info("Wakeup demo module unloaded\n");
}

module_init(wakeup_demo_init);
module_exit(wakeup_demo_exit); 