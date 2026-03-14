#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux Timer Example");
MODULE_VERSION("0.1");

#define TIMER_INTERVAL_MS 2000 // Timer interval in milliseconds

static struct timer_list my_timer;

// Timer function to be executed when the timer expires
static void my_timer_callback(struct timer_list *timer) {
    pr_info("Timer expired! Executing timer callback\n");

    // Reschedule the timer to run again after the defined interval
    mod_timer(timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));
}

static int __init example_module_init(void) {
    pr_info("Example module initializing\n");

    // Initialize the timer
    timer_setup(&my_timer, my_timer_callback, 0);

    // Set the timer to run after the defined interval
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));

    return 0;
}

static void __exit example_module_exit(void) {
    pr_info("Example module exiting\n");

    // Delete the timer before exiting the module
    del_timer(&my_timer);
}

module_init(example_module_init);
module_exit(example_module_exit);

