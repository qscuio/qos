#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define AUTHOR "qwert"
#define DESC   "Simple module show the usage of workqueue"

/*
 * The workqueue implementation include two set of API.
 * 1. schedule_xxx
 * 	Which use default working thread (events/n)
 * 2. queue_xxx
 * 	Which use user defined working thread.
 */

// Case 1: Static Declared Work on System default thread
static void static_work_fn(struct work_struct *work);
DECLARE_WORK(static_work, static_work_fn);

// Case 2: Static Declared Delayed Work on System default Thread
static void static_delayed_work_fn(struct work_struct *work);
DECLARE_DELAYED_WORK(static_delayed_work, static_delayed_work_fn);

// Case 3: Dynamic Declared Work on System default Thread
static void dynamic_work_fn(struct work_struct *work); 
static struct work_struct dynamic_work;

// Case 4: Dynamic Declared Work on System default Thread
static void dynamic_delayed_work_fn(struct work_struct *work); 
static struct delayed_work dynamic_delayed_work;

// Case 5: Static Decleared Work on User Defined Thread
static struct workqueue_struct *static_workqueue;
static void static_local_work_fn(struct work_struct *work);
DECLARE_WORK(static_local_work, static_local_work_fn);
static void static_local_delayed_work_fn(struct work_struct *work);
DECLARE_DELAYED_WORK(static_local_delayed_work, static_local_delayed_work_fn);

// Case 6: Dynamic Decleared Work on User Defined Thread
static struct workqueue_struct *dynamic_workqueue;

static void dynamic_local_work_fn(struct work_struct *work); 
static struct work_struct dynamic_local_work;
static void dynamic_local_delayed_work_fn(struct work_struct *work); 
static struct delayed_work dynamic_local_delayed_work;

// Case 7: Static Decleared Work on User Defined Single Thread
static struct workqueue_struct *static_st_workqueue;
static void static_local_st_work_fn(struct work_struct *work);
DECLARE_WORK(static_local_st_work, static_local_st_work_fn);
static void static_local_st_delayed_work_fn(struct work_struct *work);
DECLARE_DELAYED_WORK(static_local_st_delayed_work, static_local_st_delayed_work_fn);

// Case 8: Dynamic Decleared Work on User Defined Single Thread
static struct workqueue_struct *dynamic_st_workqueue;
static void dynamic_local_st_work_fn(struct work_struct *work); 
static struct work_struct dynamic_local_st_work;
static void dynamic_local_st_delayed_work_fn(struct work_struct *work); 
static struct delayed_work dynamic_local_st_delayed_work;

// Case 9: percpu work on default thread
static void __all_cpu_work_fn(struct work_struct *work);

// Case 10: percpu work on default thread
static void __cpu1_work_fn(struct work_struct *work);
DECLARE_WORK(cpu1_work, __cpu1_work_fn);
static void __cpu2_work_fn(struct work_struct *work);
DECLARE_WORK(cpu2_work, __cpu2_work_fn);

// Case 11: percpu work on user thread
static void __cpu1_local_work_fn(struct work_struct *work);
DECLARE_WORK(cpu1_local_work, __cpu1_local_work_fn);
static void __cpu2_local_work_fn(struct work_struct *work);
DECLARE_WORK(cpu2_local_work, __cpu2_local_work_fn);

static void static_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Static Work Function\n" );

    return;
}

static void static_delayed_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Static Delayed Work Function\n" );

    return;
}

static void dynamic_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Dynamic Work Function\n" );

    return;
}

static void dynamic_delayed_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Dynamic Delayed Work Function\n" );

    return;
}

static void static_local_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Static Work Function on Local thread\n" );

    return;
}

static void static_local_delayed_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Static Delayed Work Function On Local thread\n" );

    return;
}

static void static_local_st_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Static Work Function on Local single thread\n" );

    return;
}

static void static_local_st_delayed_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Static Delayed Work Function On Local single thread\n" );

    return;
}

static void dynamic_local_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Dynamic Work Function On Local thread\n" );

    return;
}

static void dynamic_local_delayed_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Dynamic Delayed Work Function On Local thread\n" );

    return;
}

static void dynamic_local_st_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Dynamic Work Function On Local single thread\n" );

    return;
}

static void dynamic_local_st_delayed_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing Dynamic Delayed Work Function On Local single thread\n" );

    return;
}

static void __cpu1_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing cpu1 work on cpu %d\n", smp_processor_id());
}


static void __cpu2_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing cpu2 work on cpu %d\n", smp_processor_id());
}

static void __cpu1_local_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing local cpu1 work on cpu %d\n", smp_processor_id());
}


static void __cpu2_local_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing local_cpu2 work on cpu %d\n", smp_processor_id());
}

static void __all_cpu_work_fn(struct work_struct *work)
{
    printk(KERN_INFO "Executing percpu work on cpu %d\n", smp_processor_id());
}

static int __init work_queue_api_init(void)
{
    printk("workqueue api demo module inserted\n");

    // Schedule static work
    schedule_work(&static_work);

    // Schedule static delayed work
    schedule_delayed_work(&static_delayed_work, 5 * HZ);

    // Schedule dynamic work
    INIT_WORK(&dynamic_work, dynamic_work_fn);
    schedule_work(&dynamic_work);

    // Schedule dynamic delayed work
    INIT_DELAYED_WORK(&dynamic_delayed_work, dynamic_delayed_work_fn);
    schedule_delayed_work(&dynamic_delayed_work, 5 * HZ);

    static_workqueue = create_workqueue("swq");
    // Schedule static work on User thread
    queue_work(static_workqueue, &static_local_work);
    queue_delayed_work(static_workqueue, &static_local_delayed_work, 10 * HZ);

    dynamic_workqueue = create_workqueue("dwq");
    // Schedule dynamic work on User thread
    INIT_WORK(&dynamic_local_work, dynamic_local_work_fn);
    queue_work(dynamic_workqueue, &dynamic_local_work);

    // Schedule dynamic delayed work on User thread
    INIT_DELAYED_WORK(&dynamic_local_delayed_work, dynamic_local_delayed_work_fn);
    queue_delayed_work(dynamic_workqueue, &dynamic_local_delayed_work, 15 * HZ);

    static_st_workqueue = create_singlethread_workqueue("sstwq");
    // Schedule static work on Single User thread
    queue_work(static_st_workqueue, &static_local_st_work);
    queue_delayed_work(static_st_workqueue, &static_local_st_delayed_work, 20 * HZ);

    dynamic_st_workqueue = create_singlethread_workqueue("dstwq");
    // Schedule dynamic delayed work on Single User thread
    INIT_WORK(&dynamic_local_st_work, dynamic_local_st_work_fn);
    queue_work(dynamic_st_workqueue, &dynamic_local_st_work);
    INIT_DELAYED_WORK(&dynamic_local_st_delayed_work, dynamic_local_st_delayed_work_fn);
    queue_delayed_work(dynamic_st_workqueue, &dynamic_local_st_delayed_work, 25 * HZ);

    // Schedule on each cpu
    schedule_work_on(1, &cpu1_work);
    schedule_work_on(2, &cpu2_work);
    queue_work_on(1, dynamic_workqueue, &cpu1_local_work);
    queue_work_on(2, dynamic_workqueue, &cpu2_local_work);

    schedule_on_each_cpu(__all_cpu_work_fn);

    show_one_workqueue(static_workqueue);
    show_one_workqueue(dynamic_workqueue);
    show_one_workqueue(static_st_workqueue);
    show_one_workqueue(dynamic_st_workqueue);
    show_all_workqueues();

    return 0;
}

static void __exit work_queue_api_exit(void)
{
    show_one_workqueue(static_workqueue);
    show_one_workqueue(dynamic_workqueue);
    show_one_workqueue(static_st_workqueue);
    show_one_workqueue(dynamic_st_workqueue);
    show_all_workqueues();
    /*
     * Understanding Workqueues and Flushing
     * Workqueues:
     *
     * Workqueues allow deferring work to be executed later in process context.
     * Work items can be regular or delayed.
     *
     * * Regular Work:
     * 		These work items are added to the workqueue and can be flushed using flush_workqueue, which waits for all queued work items to complete.
     * * Delayed Work:
     * 		These work items are scheduled to run after a certain delay.
     *
     * queue_delayed_work is used to queue delayed work items.
     *
     * Why flush_workqueue Does Not Flush Delayed Work
     *
     * * Timing: Delayed work items are scheduled to execute after a specific delay. flush_workqueue only processes work items that are ready to run. If a work item is still in its delay period, it is not considered "ready" and thus is not processed by flush_workqueue.
     *
     * * Function Design: The flush_workqueue function is designed to wait for all currently pending and running work items to complete. It does not interact with the timers or mechanisms that govern delayed work.
     */
    flush_delayed_work(&static_delayed_work);
    flush_delayed_work(&dynamic_delayed_work);
    flush_delayed_work(&static_local_delayed_work);
    flush_delayed_work(&dynamic_local_delayed_work);
    flush_delayed_work(&static_local_st_delayed_work);
    flush_delayed_work(&dynamic_local_st_delayed_work);
    flush_workqueue(static_workqueue);
    flush_workqueue(dynamic_workqueue);
    flush_workqueue(static_st_workqueue);
    flush_workqueue(dynamic_st_workqueue);
    destroy_workqueue(static_workqueue);
    destroy_workqueue(dynamic_workqueue);
    destroy_workqueue(static_st_workqueue);
    destroy_workqueue(dynamic_st_workqueue);

    printk("workqueue api demo module removed\n");
}

module_init(work_queue_api_init);
module_exit(work_queue_api_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

MODULE_VERSION("0.01");

