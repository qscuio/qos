#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux Kernel Thread Example");
MODULE_VERSION("0.1");

static struct task_struct *my_kernel_thread;

// Kernel thread function
static int my_kernel_thread_function(void *data) {
    pr_info("My kernel thread started\n");

    while (!kthread_should_stop()) {
        // This is the work that will be done in the background
        pr_info("My kernel thread is running\n");

        // Sleep for 1 second before the next iteration
        msleep(1000);
    }

    pr_info("My kernel thread stopped\n");
    return 0;
}

static int __init example_module_init(void) {
    pr_info("Example module initializing\n");

    // Create a kernel thread
    my_kernel_thread = kthread_run(my_kernel_thread_function, NULL, "my_kernel_thread");
    if (IS_ERR(my_kernel_thread)) {
        pr_err("Failed to create kernel thread\n");
        return PTR_ERR(my_kernel_thread);
    }

    return 0;
}

static void __exit example_module_exit(void) {
    pr_info("Example module exiting\n");

    // Stop the kernel thread
    if (my_kernel_thread) {
        kthread_stop(my_kernel_thread);
        my_kernel_thread = NULL;
    }
}

module_init(example_module_init);
module_exit(example_module_exit);
