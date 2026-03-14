#include <asm/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pid.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

static int pid = -1;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process ID");

// Function to get the physical address from a kernel virtual address
static unsigned long get_physical_address(unsigned long virt_addr) {
    return virt_to_phys((volatile void *)virt_addr);
}

// Function to print the physical address of pointers in task_struct
static void dump_task_struct_physical_addresses(struct task_struct *task) {
    printk(KERN_INFO "Dumping physical addresses of task_struct pointers for PID: %d\n", task->pid);

    printk(KERN_INFO "task_struct: 0x%p, physical address: 0x%lx\n", task, get_physical_address((unsigned long)task));

    printk(KERN_INFO "task->mm: 0x%p, physical address: 0x%lx\n", task->mm, get_physical_address((unsigned long)task->mm));
    printk(KERN_INFO "task->stack: 0x%p, physical address: 0x%lx\n", task->stack, get_physical_address((unsigned long)task->stack));

    printk(KERN_INFO "task->files: 0x%p, physical address: 0x%lx\n", task->files, get_physical_address((unsigned long)task->files));
    printk(KERN_INFO "task->fs: 0x%p, physical address: 0x%lx\n", task->fs, get_physical_address((unsigned long)task->fs));

    printk(KERN_INFO "task->signal: 0x%p, physical address: 0x%lx\n", task->signal, get_physical_address((unsigned long)task->signal));
    printk(KERN_INFO "task->sighand: 0x%p, physical address: 0x%lx\n", task->sighand, get_physical_address((unsigned long)task->sighand));
}

// Kernel module initialization
static int __init mymodule_init(void) {
    struct task_struct *task;

    if (pid == -1) {
        printk(KERN_ERR "Usage: insmod task_struct_dump.ko pid=<PID>\n");
        return -EINVAL;
    }

    task = get_pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        printk(KERN_ERR "Cannot find task with PID %d\n", pid);
        return -ESRCH;
    }

    dump_task_struct_physical_addresses(task);
    return 0;
}

// Kernel module cleanup
static void __exit mymodule_exit(void) {
    printk(KERN_INFO "Module unloaded\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to dump physical addresses of task_struct pointers");

