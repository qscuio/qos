#include <asm/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pid.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>

static int pid = -1;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process ID");

// Function to get the physical address from a user-space virtual address
static unsigned long get_user_physical_address(struct mm_struct *mm, unsigned long virt_addr) {
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct page *page;
    unsigned long phys_addr = 0;

    pgd = pgd_offset(mm, virt_addr);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return 0;

    p4d = p4d_offset(pgd, virt_addr);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
        return 0;

    pud = pud_offset(p4d, virt_addr);
    if (pud_none(*pud) || pud_bad(*pud))
        return 0;

    pmd = pmd_offset(pud, virt_addr);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        return 0;

    pte = pte_offset_map(pmd, virt_addr);
    if (pte_none(*pte) || !pte_present(*pte))
        return 0;

    page = pte_page(*pte);
    if (!page)
        return 0;

    phys_addr = page_to_phys(page) | (virt_addr & ~PAGE_MASK);
    pte_unmap(pte);
    return phys_addr;
}

// Function to dump the virtual and physical addresses of user-space mappings
static void dump_user_space_addresses(struct task_struct *task) {
    struct mm_struct *mm = task->mm;
    struct vm_area_struct *vma;
    unsigned long start, end, pstart, pend;

    MA_STATE(mas, &mm->mm_mt, 0, 0);

    printk(KERN_INFO "Dumping user-space addresses for PID: %d\n", task->pid);

    mas_for_each(&mas, vma, ULONG_MAX) {
        start = vma->vm_start;
        end = vma->vm_end;
	pstart = get_user_physical_address(mm, start);
	pend = get_user_physical_address(mm, end);

        printk(KERN_INFO "VMA: 0x%lx - 0x%lx, PMA: 0x%lx - 0x%lx\n", start, end, pstart, pend);
    }
}

// Kernel module initialization
static int __init mymodule_init(void) {
    struct task_struct *task;

    if (pid == -1) {
        printk(KERN_ERR "Usage: insmod user_space_dump.ko pid=<PID>\n");
        return -EINVAL;
    }

    task = get_pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        printk(KERN_ERR "Cannot find task with PID %d\n", pid);
        return -ESRCH;
    }

    dump_user_space_addresses(task);
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
MODULE_DESCRIPTION("Kernel module to dump user-space virtual and physical addresses");

