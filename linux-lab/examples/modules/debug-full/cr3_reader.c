#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/current.h>
#include <asm/io.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <linux/mm_types.h>
#include <asm/types.h>
#include <asm/processor.h>
#include<linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
#include <linux/sched/signal.h>
#endif

MODULE_LICENSE ("GPL");

static int cr3_init (void)
{
    struct task_struct* task_list;
    struct mm_struct* mm;
    void* cr3_virt;
    unsigned long cr3_phys;
    for_each_process(task_list)
    {
	mm = task_list->mm;

	/*Just skip kernel thread */
	if (mm == NULL)
	    continue;

	cr3_virt = (void*) mm->pgd;
	cr3_phys = virt_to_phys(cr3_virt);
	printk ("comm: %s\n", task_list->comm);
	printk ("cr3_virt %p, cr3_phys: %#lx\n", cr3_virt, cr3_phys);
    }

    return 0;
}

static void cr3_clean (void)
{
    printk (KERN_INFO "Goodbye\n");
}

module_init (cr3_init);
module_exit (cr3_clean);
