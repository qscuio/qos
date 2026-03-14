#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/pgtable.h>
#include <asm/pgtable.h>
#include <linux/memremap.h>
#include <asm/mmu.h>

#define NPAGES 1

static unsigned long vmalloc_area;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
extern int follow_pte(struct mm_struct *mm, unsigned long address,
                     pte_t **ptepp, spinlock_t **ptlp);
#else
extern int follow_pte(struct vm_area_struct *vma, unsigned long address,
                     pte_t **ptepp, spinlock_t **ptlp);
#endif

static int __init follow_pte_example_init(void)
{
    int ret = 0;
    struct mm_struct *mm = current->mm;
    struct vm_area_struct *vma;
    unsigned long addr;
    pte_t *pte;
    spinlock_t *ptl;

    // Allocate virtual memory
    vmalloc_area = (unsigned long)vmalloc_user(NPAGES * PAGE_SIZE);
    if (!vmalloc_area) {
        printk(KERN_ERR "Failed to allocate virtual memory.\n");
        return -ENOMEM;
    }

    // Find the VMA for the allocated area
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    down_read(&mm->mmap_sem);
#else
    mmap_read_lock(mm);
#endif
    vma = find_vma(mm, vmalloc_area);
    if (!vma || vma->vm_start > vmalloc_area) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
        up_read(&mm->mmap_sem);
#else
	mmap_read_unlock(mm);
#endif
        printk(KERN_ERR "Failed to find VMA.\n");
        vfree((void *)vmalloc_area);
        return -ENOMEM;
    }

    addr = vmalloc_area;

    // Follow PTE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
    ret = follow_pte(mm, addr, &pte, &ptl);
#else
    ret = follow_pte(vma, addr, &pte, &ptl);
#endif
    if (ret) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
        up_read(&mm->mmap_sem);
#else
	mmap_read_unlock(mm);
#endif
        printk(KERN_ERR "follow_pte failed.\n");
        vfree((void *)vmalloc_area);
        return -EFAULT;
    }

    printk(KERN_INFO "PTE for address %lx: %lx\n", addr, pte_val(*pte));
    spin_unlock(ptl);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    up_read(&mm->mmap_sem);
#else
    mmap_read_unlock(mm);
#endif

    return 0;
}

static void __exit follow_pte_example_exit(void)
{
    if (vmalloc_area) {
        vfree((void *)vmalloc_area);
        printk(KERN_INFO "Virtual memory freed.\n");
    }
}

module_init(follow_pte_example_init);
module_exit(follow_pte_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Example module demonstrating follow_pte");

