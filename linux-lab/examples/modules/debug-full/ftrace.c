#include "lookup_symbol.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <asm/ftrace.h>

// Function pointer for the original function
static asmlinkage long (*real_do_sys_fork)(const struct pt_regs *regs);

// Replacement function
static asmlinkage long my_do_sys_fork(const struct pt_regs *regs) {
    printk(KERN_INFO "my_do_sys_fork called\n");
    // Call the original do_fork function
    return real_do_sys_fork(regs);
}

// Ftrace hook structure
struct ftrace_hook {
    const char *name;
    void *function;
    void *original;
    unsigned long address;
    struct ftrace_ops ops;
};

static struct ftrace_hook hook;

// Ftrace callback handler
static void notrace ftrace_thunk(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *ops, struct ftrace_regs *regs) {
    struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
    
    if (!within_module(parent_ip, THIS_MODULE))
        return;
    
    ftrace_regs_set_instruction_pointer(regs, (unsigned long)hook->function);
}

// Register Ftrace hook
static int install_hook(struct ftrace_hook *hook) {
    int err;
    hook->address = lookup_name(hook->name);
    if (!hook->address) {
        printk(KERN_ERR "Couldn't lookup %s\n", hook->name);
        return -ENOENT;
    }
    
    *((unsigned long *)hook->original) = hook->address;
    
    hook->ops.func = ftrace_thunk;
    hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_IPMODIFY;
    
    err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
    if (err) {
        printk(KERN_ERR "ftrace_set_filter_ip failed: %d\n", err);
        return err;
    }
    
    err = register_ftrace_function(&hook->ops);
    if (err) {
        printk(KERN_ERR "register_ftrace_function failed: %d\n", err);
        return err;
    }
    
    return 0;
}

// Unregister Ftrace hook
static void remove_hook(struct ftrace_hook *hook) {
    unregister_ftrace_function(&hook->ops);
    ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
}

// Module initialization
static int __init mymodule_init(void) {
    int err;
    
    hook.name = "__do_sys_fork";
    hook.function = my_do_sys_fork;
    hook.original = &real_do_sys_fork;
    
    err = install_hook(&hook);
    if (err) {
        printk(KERN_ERR "Failed to install hook: %d\n", err);
        return err;
    }
    
    printk(KERN_INFO "Module loaded and hook installed\n");
    return 0;
}

// Module cleanup
static void __exit mymodule_exit(void) {
    remove_hook(&hook);
    printk(KERN_INFO "Module unloaded and hook removed\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Example module to hook do_fork using Ftrace");

