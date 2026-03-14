#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>
#include <linux/signal.h>

#define SIGNAL_SEND_INTERVAL 5 // Interval in seconds
#define USER_SIG SIGUSR1 // Signal to send to user-space process

static struct timer_list my_timer;
static int user_pid = -1; // PID of the user-space process to send signals to

module_param(user_pid, int, 0644);
MODULE_PARM_DESC(user_pid, "PID of the user-space process to send signals to");

static void send_signal_to_user(struct timer_list *t)
{
    struct kernel_siginfo info;
    struct task_struct *task;

    if (user_pid <= 0) {
        pr_err("Invalid user PID: %d\n", user_pid);
        return;
    }

    // Find the task_struct for the given PID
    task = pid_task(find_vpid(user_pid), PIDTYPE_PID);
    if (!task) {
        pr_err("Cannot find task for PID: %d\n", user_pid);
        return;
    }

    // Initialize the siginfo structure
    memset(&info, 0, sizeof(struct kernel_siginfo));
    info.si_signo = USER_SIG;
    info.si_code = SI_USER;
    info.si_int = 1234; // Example data to send with the signal

    // Send the signal to the user-space process
    if (send_sig_info(USER_SIG, &info, task) < 0) {
        pr_err("Failed to send signal to PID: %d\n", user_pid);
    }

    // Re-arm the timer to continue sending signals
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(SIGNAL_SEND_INTERVAL * 1000));
}

static int __init signal_module_init(void)
{
    if (user_pid <= 0) {
        pr_err("Invalid user PID: %d\n", user_pid);
        return -EINVAL;
    }

    // Initialize and start the timer
    timer_setup(&my_timer, send_signal_to_user, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(SIGNAL_SEND_INTERVAL * 1000));

    pr_info("Signal module loaded, sending signals to PID: %d\n", user_pid);
    return 0;
}

static void __exit signal_module_exit(void)
{
    del_timer(&my_timer);
    pr_info("Signal module unloaded\n");
}

module_init(signal_module_init);
module_exit(signal_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to send periodic signals to a user-space application");

