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
#include "follow_pte_helper.h"

#define NPAGES 1

static unsigned long kmalloc_area;
static unsigned long vmalloc_area;
static unsigned long vmalloc_user_area;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
extern int follow_pte(struct mm_struct *mm, unsigned long address,
                     pte_t **ptepp, spinlock_t **ptlp);
#else
extern int follow_pte(struct vm_area_struct *vma, unsigned long address,
                     pte_t **ptepp, spinlock_t **ptlp);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
static int follow_pfn(struct vm_area_struct *vma, unsigned long address,
	unsigned long *pfn)
{
	int ret = -EINVAL;
	spinlock_t *ptl;
	pte_t *ptep;

	if (!(vma->vm_flags & (VM_IO | VM_PFNMAP)))
		return ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
	ret = follow_pte(vma->vm_mm, address, &ptep, &ptl);
#else
	ret = follow_pte(vma, address, &ptep, &ptl);
#endif
	if (ret)
		return ret;
	*pfn = pte_pfn(ptep_get(ptep));
	pte_unmap_unlock(ptep, ptl);
	return 0;
}
#endif

static int __init follow_pfn_example_init(void)
{
    struct mm_struct *mm = current->mm;
    struct vm_area_struct *kvma;
    struct vm_area_struct *vvma;
    struct vm_area_struct *uvma;
    unsigned long addr;
    int ret;
    unsigned long pfn;

    // Allocate virtual memory
    kmalloc_area = (unsigned long)kmalloc(NPAGES * PAGE_SIZE, GFP_KERNEL);
    if (!kmalloc_area) {
        printk(KERN_ERR "Failed to allocate kmalloc virtual memory.\n");
    }

    // Allocate virtual memory
    vmalloc_area = (unsigned long)vmalloc(NPAGES * PAGE_SIZE);
    if (!vmalloc_area) {
	kfree((void *)kmalloc_area);
        printk(KERN_ERR "Failed to allocate vmalloc virtual memory.\n");
    }

    // Allocate virtual memory
    vmalloc_user_area = (unsigned long)vmalloc_user(NPAGES * PAGE_SIZE);
    if (!vmalloc_user_area) {
	kfree((void *)kmalloc_area);
	vfree((void *)vmalloc_area);
        printk(KERN_ERR "Failed to allocate vmalloc user virtual memory.\n");
    }

    // Find the VMA for the allocated area
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    down_read(&mm->mmap_sem);
#else
    mmap_read_lock(mm);
#endif
    kvma = find_vma(mm, kmalloc_area);
    if (!kvma)
    {
	printk("Failed  to find kvma for %#lx\n", kmalloc_area);
    }

    vvma = find_vma(mm, vmalloc_area);
    if (!vvma)
    {
	printk("Failed  to find vvma for %#lx\n", vmalloc_area);
    }

    uvma = find_vma(mm, vmalloc_user_area);
    if (!uvma)
    {
	printk("Failed  to find uvma for %#lx\n", vmalloc_user_area);
    }

    if (!vvma || vvma->vm_start > vmalloc_area) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
        up_read(&mm->mmap_sem);
#else
	mmap_read_unlock(mm);
#endif
        printk(KERN_ERR "Failed to find VMA.\n");
	kfree((void *)kmalloc_area);
	vfree((void *)vmalloc_area);
	vfree((void *)vmalloc_user_area);
        return -ENOMEM;
    }

    addr = vmalloc_area;

    // Follow PFN
    ret = follow_pfn(vvma, addr, &pfn);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    up_read(&mm->mmap_sem);
#else
    mmap_read_unlock(mm);
#endif
    if (ret) {
        printk(KERN_ERR "follow_pfn failed: %d\n", ret);
	kfree((void *)kmalloc_area);
	vfree((void *)vmalloc_area);
	vfree((void *)vmalloc_user_area);
        return ret;
    }

    kfree((void *)kmalloc_area);
    vfree((void *)vmalloc_area);
    vfree((void *)vmalloc_user_area);

    printk(KERN_INFO "PFN for address %lx: %lx\n", addr, pfn);

    return 0;
}

static void __exit follow_pfn_example_exit(void)
{
    if (vmalloc_area) {
        vfree((void *)vmalloc_area);
        printk(KERN_INFO "Virtual memory freed.\n");
    }
}

module_init(follow_pfn_example_init);
module_exit(follow_pfn_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Example module demonstrating follow_pfn");

