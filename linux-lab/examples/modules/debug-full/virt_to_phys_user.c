#include <asm/io.h>
#include <asm/page.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>

static int pid = -1;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process ID");

static unsigned long user_address = 0;
module_param(user_address, ulong, 0);
MODULE_PARM_DESC(user_address, "User-space virtual address");

// Function to get the physical address from a user-space virtual address
static void print_user_virtual_to_physical(unsigned long virt_addr, pid_t pid) {
    struct task_struct *task;
    struct mm_struct *mm;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct page *page;
    unsigned long phys_addr = 0;

    // Find the task by PID
    task = get_pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        printk(KERN_ERR "Cannot find task with PID %d\n", pid);
        return;
    }

    mm = task->mm;
    if (!mm) {
        printk(KERN_ERR "Cannot get mm_struct for PID %d\n", pid);
        return;
    }

    down_read(&mm->mmap_lock);

    pgd = pgd_offset(mm, virt_addr);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        goto out;

    p4d = p4d_offset(pgd, virt_addr);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
        goto out;

    pud = pud_offset(p4d, virt_addr);
    if (pud_none(*pud) || pud_bad(*pud))
        goto out;

    pmd = pmd_offset(pud, virt_addr);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        goto out;

    pte = pte_offset_map(pmd, virt_addr);
    if (pte_none(*pte))
        goto out;

    if (!pte_present(*pte))
        goto out;

    page = pte_page(*pte);
    if (!page)
        goto out;

    phys_addr = page_to_phys(page) | (virt_addr & ~PAGE_MASK);

out:
    up_read(&mm->mmap_lock);
    printk(KERN_INFO "User virtual address: 0x%lx, Physical address: 0x%lx\n", virt_addr, phys_addr);
}

// Kernel module initialization
static int __init mymodule_init(void) {
    if (pid == -1 || user_address == 0) {
        printk(KERN_ERR "Usage: insmod user_virt_to_phys_example.ko pid=<PID> user_address=<USER_ADDRESS>\n");
        return -EINVAL;
    }

    print_user_virtual_to_physical(user_address, pid);
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
MODULE_DESCRIPTION("Kernel module to print user-space virtual to physical address mapping");
