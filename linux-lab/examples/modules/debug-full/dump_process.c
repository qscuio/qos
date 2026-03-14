#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/sched/mm.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/sched.h>
#include <linux/string_helpers.h>
#include <linux/kprobes.h>
#include "lookup_symbol.h"

/*
 * 在Linux中，内核线程（kthread）没有关联的`mm_struct`（`task->mm`）存在几个原因：
 *
 * 1. **内存上下文**：
 *    - 内核线程专门在内核空间执行，不涉及用户空间代码。因此，它们不需要像用户空间进程那样的地址空间（`mm_struct`）。
 *    - `mm_struct`包含有关内存映射、页表和其他与用户空间进程相关的内存详细信息。由于内核线程没有用户空间上下文，因此它们不需要这些结构。
 *
 * 2. **执行简化**：
 *    - 内核线程通常用于执行内核级任务，如后台任务、维护活动和I/O处理。
 *    - 它们的执行上下文由内核调度程序管理，并且它们不直接与用户空间内存交互。
 *
 * 3. **创建和管理**：
 *    - 内核线程通过`kthread_create()`或`kthread_run()`等函数创建，这些函数在内核中管理其生命周期和调度。
 *    - 这些函数分配必要的资源并初始化线程的控制结构（`task_struct`），但不将它们与用户空间地址空间（`mm_struct`）关联起来。
 *
 * 4. **性能和效率**：
 *    - 由于没有关联的`mm_struct`，内核线程避免了管理用户空间内存上下文和在用户空间和内核空间模式之间切换的开销。
 *    - 这种简化提高了内核线程的效率和性能，使它们适用于不需要用户空间访问或内存管理的任务。
 *
 *  ### 访问内核线程信息
 *
 *  在开发Linux内核模块时，与内核线程交互时需要注意以下几点：
 *
 *  - **线程识别**：使用`task_struct`中的`PF_KTHREAD`标志来编程识别内核线程。
 *                           
 *  - **线程信息**：通过`task_struct`中的字段和相关的内核API（如`kthread_create_info`等）获取特定于内核线程的信息。
 *
 *  - **资源管理**：与内核线程交互时确保适当的资源管理实践，因为它们受到内核调度和系统范围资源限制的影响。
 *
 *  通过理解这些区别，内核开发人员可以有效地利用内核线程执行专门的任务，而无需处理用户空间进程管理的开销。
 */

static char command_line[PAGE_SIZE] = {0};

// kernel/linux-6.4.3/fs/proc/array.c
//
struct kthread {
        unsigned long flags;
        unsigned int cpu;
        int result;
        int (*threadfn)(void *);
        void *data;
        struct completion parked;
        struct completion exited;
#ifdef CONFIG_BLK_CGROUP
        struct cgroup_subsys_state *blkcg_css;
#endif
        /* To store the full name if task comm is truncated. */
        char *full_name;
};

static int (*get_cmdline_ptr)(struct task_struct *task, char *buffer, int buflen);

static int __init dump_process_init(void)
{
    unsigned long addr;
    
    addr = lookup_name("get_cmdline");
    if (!addr) {
        pr_err("Failed to find get_cmdline symbol\n");
        return -EINVAL;
    }
    get_cmdline_ptr = (void *)addr;
    
    int res = 0;
    struct task_struct *task;
    struct mm_struct *mm;
    struct kthread *ktask;

    printk(KERN_INFO "Dumping all processes:\n");

    for_each_process(task) {
	mm = get_task_mm(task);
	if (mm) {
	    res = get_cmdline_ptr(task, command_line, PAGE_SIZE-1);;
	    if (res) {
		command_line[res] = '\0';
		printk(KERN_INFO "[USER] PID: %d, State: %d, Parent PID: %d, Name: %s, Full Command: %s\n",
			task->pid, task->__state, task->real_parent->pid, task->comm, command_line);
	    }
	}
	else if (task->flags & PF_KTHREAD) 
	{ 
	    ktask = task->worker_private;
	    if (ktask)
		printk(KERN_INFO "KThread PID: %d, Full Name: %s\n", task->pid, ktask->full_name);
	    else
		printk(KERN_INFO "KThread PID: %d, Name: %s\n", task->pid, task->comm);
	}
	else
	{
	    printk(KERN_INFO "PID: %d, State: %d, Parent PID: %d, Name: %s, Full Command: %s\n",
		    task->pid, task->__state, task->real_parent->pid, task->comm, "No MM");
	}
    }

    return 0;
}

static void __exit dump_processes_exit(void) {
    printk(KERN_INFO "Module unloaded\n");
}

module_init(dump_process_init);
module_exit(dump_processes_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to dump all processes with full command");

