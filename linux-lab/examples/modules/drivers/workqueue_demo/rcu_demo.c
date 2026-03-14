#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/rculist.h>
#include <linux/spinlock.h>
#include <linux/completion.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RCU Demo");
MODULE_DESCRIPTION("Demonstration of Linux kernel RCU mechanisms");

/* RCU-protected data structures */
struct demo_node {
    int value;
    struct list_head list;     /* For standard list */
    struct hlist_node hnode;   /* For hash list */
    struct rcu_head rcu_head;  /* For callback */
    char data[32];
};

/* Global data structures */
static LIST_HEAD(demo_list);           /* RCU-protected list */
static HLIST_HEAD(demo_hlist);         /* RCU-protected hash list */
static struct demo_node *global_ptr;   /* RCU-protected pointer */

/* Synchronization */
static DEFINE_SPINLOCK(demo_lock);
static atomic_t reader_count = ATOMIC_INIT(0);
static atomic_t updater_count = ATOMIC_INIT(0);
static atomic_t callback_count = ATOMIC_INIT(0);

/* Thread control */
static struct task_struct *reader_thread;
static struct task_struct *updater_thread;
static struct task_struct *list_thread;
static struct task_struct *hlist_thread;
static bool should_stop;

/* Procfs entries */
static struct proc_dir_entry *demo_dir;
static struct proc_dir_entry *stats_entry;
static struct proc_dir_entry *control_entry;

/* RCU callback demonstration */
static void demo_rcu_callback(struct rcu_head *head)
{
    struct demo_node *node = container_of(head, struct demo_node, rcu_head);
    atomic_inc(&callback_count);
    pr_info("RCU callback: freeing node with value %d\n", node->value);
    kfree(node);
}

/* List manipulation functions */
static void add_to_list(int value)
{
    struct demo_node *node;
    
    node = kzalloc(sizeof(*node), GFP_KERNEL);
    if (!node)
        return;
    
    node->value = value;
    snprintf(node->data, sizeof(node->data), "Node-%d", value);
    
    spin_lock(&demo_lock);
    list_add_rcu(&node->list, &demo_list);
    spin_unlock(&demo_lock);
}

static void remove_from_list(int value)
{
    struct demo_node *node, *tmp;
    
    spin_lock(&demo_lock);
    list_for_each_entry_safe(node, tmp, &demo_list, list) {
        if (node->value == value) {
            list_del_rcu(&node->list);
            spin_unlock(&demo_lock);
            /* Use callback for cleanup */
            call_rcu(&node->rcu_head, demo_rcu_callback);
            return;
        }
    }
    spin_unlock(&demo_lock);
}

/* Hash list manipulation functions */
static void add_to_hlist(int value)
{
    struct demo_node *node;
    
    node = kzalloc(sizeof(*node), GFP_KERNEL);
    if (!node)
        return;
    
    node->value = value;
    snprintf(node->data, sizeof(node->data), "HNode-%d", value);
    
    spin_lock(&demo_lock);
    hlist_add_head_rcu(&node->hnode, &demo_hlist);
    spin_unlock(&demo_lock);
}

static void remove_from_hlist(int value)
{
    struct demo_node *node;
    struct hlist_node *tmp;
    
    spin_lock(&demo_lock);
    hlist_for_each_entry_safe(node, tmp, &demo_hlist, hnode) {
        if (node->value == value) {
            hlist_del_rcu(&node->hnode);
            spin_unlock(&demo_lock);
            call_rcu(&node->rcu_head, demo_rcu_callback);
            return;
        }
    }
    spin_unlock(&demo_lock);
}

/* Global pointer update demonstration */
static void update_global_ptr(int value)
{
    struct demo_node *old_ptr, *new_ptr;
    
    new_ptr = kzalloc(sizeof(*new_ptr), GFP_KERNEL);
    if (!new_ptr)
        return;
    
    new_ptr->value = value;
    snprintf(new_ptr->data, sizeof(new_ptr->data), "Global-%d", value);
    
    spin_lock(&demo_lock);
    old_ptr = rcu_dereference_protected(global_ptr, lockdep_is_held(&demo_lock));
    rcu_assign_pointer(global_ptr, new_ptr);
    spin_unlock(&demo_lock);
    
    if (old_ptr)
        call_rcu(&old_ptr->rcu_head, demo_rcu_callback);
}

/* Reader thread function */
static int reader_thread_fn(void *data)
{
    struct demo_node *node;
    
    while (!kthread_should_stop() && !should_stop) {
        rcu_read_lock();
        atomic_inc(&reader_count);
        
        /* Read global pointer */
        node = rcu_dereference(global_ptr);
        if (node)
            pr_info("Reader: global_ptr value = %d\n", node->value);
        
        /* Read from list */
        list_for_each_entry_rcu(node, &demo_list, list)
            pr_info("Reader: list node value = %d\n", node->value);
        
        /* Read from hash list */
        hlist_for_each_entry_rcu(node, &demo_hlist, hnode)
            pr_info("Reader: hlist node value = %d\n", node->value);
        
        rcu_read_unlock();
        
        msleep_interruptible(1000);
    }
    
    return 0;
}

/* Updater thread function */
static int updater_thread_fn(void *data)
{
    int value = 0;
    
    while (!kthread_should_stop() && !should_stop) {
        atomic_inc(&updater_count);
        
        /* Update global pointer */
        update_global_ptr(value);
        
        /* Update list */
        add_to_list(value);
        if (value > 5)
            remove_from_list(value - 5);
        
        /* Update hash list */
        add_to_hlist(value);
        if (value > 5)
            remove_from_hlist(value - 5);
        
        value++;
        msleep_interruptible(2000);
    }
    
    return 0;
}

/* List traversal thread */
static int list_thread_fn(void *data)
{
    struct demo_node *node;
    int count;
    
    while (!kthread_should_stop() && !should_stop) {
        count = 0;
        rcu_read_lock();
        
        /* Traverse list with different RCU list macros */
        list_for_each_entry_rcu(node, &demo_list, list)
            count++;
        
        pr_info("List count: %d\n", count);
        rcu_read_unlock();
        
        msleep_interruptible(3000);
    }
    
    return 0;
}

/* Hash list traversal thread */
static int hlist_thread_fn(void *data)
{
    struct demo_node *node;
    int count;
    
    while (!kthread_should_stop() && !should_stop) {
        count = 0;
        rcu_read_lock();
        
        /* Traverse hash list with different RCU hlist macros */
        hlist_for_each_entry_rcu(node, &demo_hlist, hnode)
            count++;
        
        pr_info("Hash list count: %d\n", count);
        rcu_read_unlock();
        
        msleep_interruptible(3000);
    }
    
    return 0;
}

/* Procfs handlers */
static ssize_t stats_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[512];
    int len;
    
    len = snprintf(buf, sizeof(buf),
                  "RCU Statistics:\n"
                  "Reader operations: %d\n"
                  "Updater operations: %d\n"
                  "RCU callbacks executed: %d\n",
                  atomic_read(&reader_count),
                  atomic_read(&updater_count),
                  atomic_read(&callback_count));
    
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
    int value;
    
    buf_size = min(count, sizeof(buf) - 1);
    if (copy_from_user(buf, ubuf, buf_size))
        return -EFAULT;
    
    buf[buf_size] = '\0';
    
    if (sscanf(buf, "add %d", &value) == 1) {
        add_to_list(value);
        add_to_hlist(value);
        pr_info("Added value %d to lists\n", value);
    }
    else if (sscanf(buf, "remove %d", &value) == 1) {
        remove_from_list(value);
        remove_from_hlist(value);
        pr_info("Removed value %d from lists\n", value);
    }
    else if (sscanf(buf, "update %d", &value) == 1) {
        update_global_ptr(value);
        pr_info("Updated global pointer to value %d\n", value);
    }
    else if (strncmp(buf, "sync", 4) == 0) {
        /* Demonstrate different RCU synchronization methods */
        synchronize_rcu();
        pr_info("Completed synchronize_rcu()\n");
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

static int __init rcu_demo_init(void)
{
    /* Initialize global pointer */
    rcu_assign_pointer(global_ptr, NULL);
    
    /* Create procfs entries */
    demo_dir = proc_mkdir("rcu_demo", NULL);
    if (!demo_dir)
        goto err_dir;
    
    stats_entry = proc_create("stats", 0444, demo_dir, &stats_ops);
    if (!stats_entry)
        goto err_stats;
    
    control_entry = proc_create("control", 0220, demo_dir, &control_ops);
    if (!control_entry)
        goto err_control;
    
    /* Start threads */
    reader_thread = kthread_run(reader_thread_fn, NULL, "rcu_reader");
    if (IS_ERR(reader_thread))
        goto err_reader;
    
    updater_thread = kthread_run(updater_thread_fn, NULL, "rcu_updater");
    if (IS_ERR(updater_thread))
        goto err_updater;
    
    list_thread = kthread_run(list_thread_fn, NULL, "rcu_list");
    if (IS_ERR(list_thread))
        goto err_list;
    
    hlist_thread = kthread_run(hlist_thread_fn, NULL, "rcu_hlist");
    if (IS_ERR(hlist_thread))
        goto err_hlist;
    
    pr_info("RCU demo module loaded\n");
    return 0;

err_hlist:
    kthread_stop(list_thread);
err_list:
    kthread_stop(updater_thread);
err_updater:
    kthread_stop(reader_thread);
err_reader:
    proc_remove(control_entry);
err_control:
    proc_remove(stats_entry);
err_stats:
    proc_remove(demo_dir);
err_dir:
    return -ENOMEM;
}

static void cleanup_list(void)
{
    struct demo_node *node, *tmp;
    
    /* Clean up standard list */
    spin_lock(&demo_lock);
    list_for_each_entry_safe(node, tmp, &demo_list, list) {
        list_del_rcu(&node->list);
        synchronize_rcu();
        kfree(node);
    }
    spin_unlock(&demo_lock);
}

static void cleanup_hlist(void)
{
    struct demo_node *node;
    struct hlist_node *tmp;
    
    /* Clean up hash list */
    spin_lock(&demo_lock);
    hlist_for_each_entry_safe(node, tmp, &demo_hlist, hnode) {
        hlist_del_rcu(&node->hnode);
        synchronize_rcu();
        kfree(node);
    }
    spin_unlock(&demo_lock);
}

static void __exit rcu_demo_exit(void)
{
    struct demo_node *old_ptr;
    
    /* Stop threads */
    should_stop = true;
    kthread_stop(hlist_thread);
    kthread_stop(list_thread);
    kthread_stop(updater_thread);
    kthread_stop(reader_thread);
    
    /* Remove procfs entries */
    proc_remove(control_entry);
    proc_remove(stats_entry);
    proc_remove(demo_dir);
    
    /* Clean up data structures */
    cleanup_list();
    cleanup_hlist();
    
    /* Clean up global pointer */
    spin_lock(&demo_lock);
    old_ptr = global_ptr;
    rcu_assign_pointer(global_ptr, NULL);
    spin_unlock(&demo_lock);
    
    if (old_ptr) {
        synchronize_rcu();
        kfree(old_ptr);
    }
    
    pr_info("RCU demo module unloaded\n");
}

module_init(rcu_demo_init);
module_exit(rcu_demo_exit); 