#include <linux/module.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm_types.h>

#define NPAGES 16

struct page **pages;

static int my_func(pte_t *pte, unsigned long addr, void *data)
{
    struct page *page = pte_page(*pte);
    printk(KERN_INFO "Applying function to page at address: %lx, page: %p\n", addr, page);
    return 0;
}

static int __init apply_to_page_range_init(void)
{
    int ret;
    unsigned long addr;
#if 0
    struct vm_area_struct *vma;
#endif

    pages = kmalloc_array(NPAGES, sizeof(struct page *), GFP_KERNEL);
    if (!pages)
        return -ENOMEM;

    for (int i = 0; i < NPAGES; i++) {
        pages[i] = alloc_page(GFP_KERNEL);
        if (!pages[i]) {
            ret = -ENOMEM;
            goto out_free_pages;
        }
        memset(page_address(pages[i]), 0, PAGE_SIZE);
    }

    addr = (unsigned long)vmalloc_user(NPAGES * PAGE_SIZE);
    if (!addr) {
        ret = -ENOMEM;
        goto out_free_pages;
    }

#if 0
    vma = find_vma(current->mm, addr);
    if (!vma) {
        ret = -ENOMEM;
        goto out_vfree;
    }
#endif

    ret = apply_to_existing_page_range(current->mm, addr, NPAGES * PAGE_SIZE, my_func, NULL);
    if (ret)
        goto out_vfree;

    return 0;

out_vfree:
    vfree((void *)addr);
out_free_pages:
    for (int i = 0; i < NPAGES; i++) {
        if (pages[i])
            __free_page(pages[i]);
    }
    kfree(pages);
    return ret;
}

static void __exit apply_to_page_range_exit(void)
{
    for (int i = 0; i < NPAGES; i++) {
        if (pages[i])
            __free_page(pages[i]);
    }
    kfree(pages);
    printk(KERN_INFO "Module exiting, cleaned up allocated pages.\n");
}

module_init(apply_to_page_range_init);
module_exit(apply_to_page_range_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("Example module demonstrating apply_to_existing_page_range");

