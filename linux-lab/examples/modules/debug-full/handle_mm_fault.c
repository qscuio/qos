#include <linux/module.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/mm_types.h>
#include <linux/sched/signal.h>
#include <linux/vmalloc.h>
#include <linux/version.h>

#define NPAGES 16

static unsigned long vmalloc_area;

static int __init handle_mm_fault_example_init(void)
{
    struct mm_struct *mm = current->mm;
    unsigned long addr;
    struct page *page;
    struct vm_area_struct *vma;
    int ret;

    // Allocate virtual memory
    vmalloc_area = (unsigned long)vmalloc_user(NPAGES * PAGE_SIZE);
    if (!vmalloc_area) {
        printk(KERN_ERR "Failed to allocate virtual memory.\n");
        return -ENOMEM;
    }

    // Find the VMA for the allocated area
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    down_write(&mm->mmap_sem);
#else
    mmap_write_lock(mm);
#endif
    vma = find_vma(mm, vmalloc_area);
    if (!vma || vma->vm_start > vmalloc_area) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
	up_write(&mm->mmap_sem);
#else
	mmap_write_unlock(mm);
#endif
        printk(KERN_ERR "Failed to find VMA.\n");
        vfree((void *)vmalloc_area);
        return -ENOMEM;
    }

    // Access a page in the allocated range to trigger a page fault
    addr = vmalloc_area + PAGE_SIZE;
    ret = get_user_pages_remote(mm, addr, 1, FOLL_WRITE | FOLL_FORCE, &page, NULL);
    if (ret <= 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
        up_write(&mm->mmap_sem);
#else
	mmap_write_unlock(mm);
#endif
        printk(KERN_ERR "Failed to get user pages: %d\n", ret);
        vfree((void *)vmalloc_area);
        return -EFAULT;
    }

    // Handle the page fault
    ret = handle_mm_fault(vma, addr, FAULT_FLAG_WRITE | FAULT_FLAG_ALLOW_RETRY, NULL);
    if (ret & VM_FAULT_ERROR) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
        up_write(&mm->mmap_sem);
#else
	mmap_write_unlock(mm);
#endif
        printk(KERN_ERR "handle_mm_fault failed: %d\n", ret);
        put_page(page);
        vfree((void *)vmalloc_area);
        return -EFAULT;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    up_write(&mm->mmap_sem);
#else
    mmap_write_unlock(mm);
#endif

    printk(KERN_INFO "Page fault handled successfully.\n");

    // Cleanup
    put_page(page);
    return 0;
}

static void __exit handle_mm_fault_example_exit(void)
{
    if (vmalloc_area) {
        vfree((void *)vmalloc_area);
        printk(KERN_INFO "Virtual memory freed.\n");
    }
}

module_init(handle_mm_fault_example_init);
module_exit(handle_mm_fault_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("Example module demonstrating handle_mm_fault");

