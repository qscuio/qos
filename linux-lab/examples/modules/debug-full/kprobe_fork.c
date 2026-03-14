#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

// Prototype for the original do_fork function
static long (*original_do_fork)(unsigned long, unsigned long, unsigned long, int, struct task_struct *, int *);

// Replacement function
static long my_do_fork(unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size, int __user *parent_tidptr, struct task_struct *p, int *child_tidptr) {
    printk(KERN_INFO "my_do_fork called with clone_flags: %lx\n", clone_flags);
    // Call the original do_fork function
    return original_do_fork(clone_flags, stack_start, stack_size, *parent_tidptr, p, child_tidptr);
}

// Kprobe for do_fork
static struct kprobe kp = {
    .symbol_name = "do_fork"
};

// Pre-handler: called just before the probed instruction is executed
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    // Modify the original function pointer to our new function
    original_do_fork = (void *) kp.addr;
    *((unsigned long *) kp.addr) = (unsigned long) my_do_fork;
    return 0;
}

// Initialization function
static int __init kprobe_init(void) {
    int ret;

    kp.pre_handler = handler_pre;

    // Register the kprobe
    ret = register_kprobe(&kp);
    if (ret < 0) {
        printk(KERN_ERR "register_kprobe failed, returned %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "kprobe registered\n");
    return 0;
}

// Cleanup function
static void __exit kprobe_exit(void) {
    unregister_kprobe(&kp);
    printk(KERN_INFO "kprobe unregistered\n");
}

module_init(kprobe_init)
module_exit(kprobe_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to hook do_fork using kprobes");

