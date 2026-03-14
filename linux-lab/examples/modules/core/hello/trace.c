#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/sched.h>

static void *my_kmalloc(size_t size, gfp_t flags)
{
    struct task_struct *task = current;
    printk(KERN_INFO "kmalloc called from %s (pid: %d)\n", task->comm, task->pid);
    return kmalloc(size, flags);
}

static void my_kfree(void *ptr)
{
    struct task_struct *task = current;
    printk(KERN_INFO "kfree called from %s (pid: %d)\n", task->comm, task->pid);
    kfree(ptr);
}

static int __init trace_malloc_init(void)
{
    printk(KERN_INFO "Replacing kmalloc and kfree\n");
    preempt_disable();
    set_current_state(TASK_UNINTERRUPTIBLE);
    schedule_timeout(HZ/10);
    *(unsigned long *)kmalloc = *(unsigned long *)my_kmalloc;
    *(unsigned long *)kfree = *(unsigned long *)my_kfree;
    set_current_state(TASK_RUNNING);
    preempt_enable();
    return 0;
}

static void __exit trace_malloc_exit(void)
{
    printk(KERN_INFO "Restoring original kmalloc and kfree\n");
    preempt_disable();
    set_current_state(TASK_UNINTERRUPTIBLE);
    schedule_timeout(HZ/10);
    *(unsigned long *)kmalloc = *(unsigned long *)(kmalloc+1);
    *(unsigned long *)kfree = *(unsigned long *)(kfree+1);
    set_current_state(TASK_RUNNING);
    preempt_enable();
}

module_init(trace_malloc_init);
module_exit(trace_malloc_exit);
MODULE_LICENSE("GPL");

