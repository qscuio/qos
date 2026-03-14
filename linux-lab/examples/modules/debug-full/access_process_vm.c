#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>
#include <linux/version.h>

static int target_pid = -1;
module_param(target_pid, int, 0644);
MODULE_PARM_DESC(target_pid, "Target process PID");

static unsigned long target_addr = 0;
module_param(target_addr, ulong, 0644);
MODULE_PARM_DESC(target_addr, "Target address in the process's address space");

static int __init access_process_vm_example_init(void)
{
    struct task_struct *task;
    struct mm_struct *mm;
    char buf[100];
    int ret;

    printk(KERN_INFO "Initializing access_process_vm example module.\n");

    if (target_pid == -1 || target_addr == 0) {
        printk(KERN_ERR "Please specify target_pid and target_addr module parameters.\n");
        return -EINVAL;
    }

    task = get_pid_task(find_get_pid(target_pid), PIDTYPE_PID);
    if (!task) {
        printk(KERN_ERR "Failed to find task for PID %d.\n", target_pid);
        return -ESRCH;
    }

    mm = task->mm;
    if (!mm) {
        printk(KERN_ERR "Failed to get mm_struct for task.\n");
        put_task_struct(task);
        return -EINVAL;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    down_read(&mm->mmap_sem);
#else
    mmap_read_lock(mm);
#endif

    // Read from target process's memory
    ret = access_process_vm(task, target_addr, buf, sizeof(buf), FOLL_FORCE);
    if (ret <= 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
        up_read(&mm->mmap_sem);
#else
	mmap_read_unlock(mm);
#endif
        put_task_struct(task);
        printk(KERN_ERR "Failed to read from process memory: %d\n", ret);
        return ret;
    }
    buf[ret] = '\0';  // Null-terminate the buffer

    printk(KERN_INFO "Read %d bytes from address %lx: %s\n", ret, target_addr, buf);

    // Write to target process's memory
    snprintf(buf, sizeof(buf), "Hello from kernel module!");
    ret = access_process_vm(task, target_addr, buf, strlen(buf) + 1, FOLL_FORCE);
    if (ret <= 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
        up_read(&mm->mmap_sem);
#else
	mmap_read_unlock(mm);
#endif
        put_task_struct(task);
        printk(KERN_ERR "Failed to write to process memory: %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "Wrote %d bytes to address %lx: %s\n", ret, target_addr, buf);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    up_read(&mm->mmap_sem);
#else
    mmap_read_unlock(mm);
#endif
    put_task_struct(task);

    return 0;
}

static void __exit access_process_vm_example_exit(void)
{
    printk(KERN_INFO "Exiting access_process_vm example module.\n");
}

module_init(access_process_vm_example_init);
module_exit(access_process_vm_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Example module demonstrating access_process_vm");

