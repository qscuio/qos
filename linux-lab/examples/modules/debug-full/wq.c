#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux Workqueue Example");
MODULE_VERSION("0.1");

static struct workqueue_struct *example_workqueue;
static struct work_struct example_work;

// The work function to be executed by the workqueue
static void example_work_handler(struct work_struct *work) {
    // This is the work that will be done in the background
    pr_info("Example work item executed\n");
}

static int __init example_module_init(void) {
    pr_info("Example module initializing\n");

    // Create a workqueue for our module
    example_workqueue = create_singlethread_workqueue("example_workqueue");
    if (!example_workqueue) {
        pr_err("Failed to create workqueue\n");
        return -ENOMEM;
    }

    // Initialize the work item and bind it to the work handler function
    INIT_WORK(&example_work, example_work_handler);

    // Schedule the work item to be executed
    queue_work(example_workqueue, &example_work);

    return 0;
}

static void __exit example_module_exit(void) {
    // Flush and destroy the workqueue
    flush_workqueue(example_workqueue);
    destroy_workqueue(example_workqueue);

    pr_info("Example module exiting\n");
}

module_init(example_module_init);
module_exit(example_module_exit);

