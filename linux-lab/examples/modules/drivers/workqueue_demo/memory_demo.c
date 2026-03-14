#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/mm_types.h>
#include <linux/memblock.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/dma-mapping.h>
#include <linux/page-flags.h>
#include <linux/hugetlb.h>
#include <asm/pgtable.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Memory Demo");
MODULE_DESCRIPTION("Demonstration of Linux kernel memory management concepts");

/* 全局变量用于演示不同类型的内存分配 */
static void *kmalloc_mem;
static void *vmalloc_mem;
static struct page *alloc_pages_mem;
static void *high_mem;
static void *dma_mem;
static dma_addr_t dma_handle;
static struct proc_dir_entry *demo_dir;
static struct proc_dir_entry *stats_entry;
static struct proc_dir_entry *control_entry;

#define DEMO_SIZE PAGE_SIZE * 4
#define HIGH_ORDER 2

/* 演示页表遍历 */
static void walk_page_table(unsigned long addr)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct mm_struct *mm = current->mm;

    pgd = pgd_offset(mm, addr);
    if (pgd_none(*pgd) || pgd_bad(*pgd)) {
        pr_info("Invalid PGD\n");
        return;
    }

    p4d = p4d_offset(pgd, addr);
    if (p4d_none(*p4d) || p4d_bad(*p4d)) {
        pr_info("Invalid P4D\n");
        return;
    }

    pud = pud_offset(p4d, addr);
    if (pud_none(*pud) || pud_bad(*pud)) {
        pr_info("Invalid PUD\n");
        return;
    }

    pmd = pmd_offset(pud, addr);
    if (pmd_none(*pmd) || pmd_bad(*pmd)) {
        pr_info("Invalid PMD\n");
        return;
    }

    pte = pte_offset_kernel(pmd, addr);
    if (!pte_none(*pte)) {
        unsigned long pfn = pte_pfn(*pte);
        pr_info("Virtual Address: 0x%lx -> Physical Address: 0x%lx\n",
                addr, (pfn << PAGE_SHIFT) | (addr & ~PAGE_MASK));
        pr_info("Page flags: %s%s%s%s%s\n",
                pte_present(*pte) ? "Present " : "",
                pte_write(*pte) ? "Writable " : "",
                pte_young(*pte) ? "Accessed " : "",
                pte_dirty(*pte) ? "Dirty " : "",
                pte_special(*pte) ? "Special " : "");
    }
}

/* 演示不同类型的内存分配 */
static int allocate_memory(void)
{
    /* 1. kmalloc - 物理地址连续的内存 */
    kmalloc_mem = kmalloc(DEMO_SIZE, GFP_KERNEL);
    if (!kmalloc_mem) {
        pr_err("kmalloc failed\n");
        goto err_kmalloc;
    }
    pr_info("kmalloc allocated at virtual addr: 0x%px\n", kmalloc_mem);

    /* 2. vmalloc - 虚拟地址连续的内存 */
    vmalloc_mem = vmalloc(DEMO_SIZE);
    if (!vmalloc_mem) {
        pr_err("vmalloc failed\n");
        goto err_vmalloc;
    }
    pr_info("vmalloc allocated at virtual addr: 0x%px\n", vmalloc_mem);

    /* 3. alloc_pages - 直接页面分配 */
    alloc_pages_mem = alloc_pages(GFP_KERNEL, HIGH_ORDER);
    if (!alloc_pages_mem) {
        pr_err("alloc_pages failed\n");
        goto err_pages;
    }
    pr_info("alloc_pages allocated at page addr: 0x%px\n", alloc_pages_mem);

    /* 4. 高端内存映射演示 */
    if (PageHighMem(alloc_pages_mem)) {
        high_mem = kmap(alloc_pages_mem);
        if (!high_mem) {
            pr_err("kmap failed\n");
            goto err_kmap;
        }
        pr_info("High memory mapped at virtual addr: 0x%px\n", high_mem);
    }

    /* 5. DMA内存分配演示 */
    dma_mem = dma_alloc_coherent(NULL, DEMO_SIZE, &dma_handle, GFP_KERNEL);
    if (!dma_mem) {
        pr_err("dma_alloc_coherent failed\n");
        goto err_dma;
    }
    pr_info("DMA memory allocated at virtual addr: 0x%px, bus addr: 0x%llx\n",
            dma_mem, (unsigned long long)dma_handle);

    return 0;

err_dma:
    if (PageHighMem(alloc_pages_mem))
        kunmap(alloc_pages_mem);
err_kmap:
    __free_pages(alloc_pages_mem, HIGH_ORDER);
err_pages:
    vfree(vmalloc_mem);
err_vmalloc:
    kfree(kmalloc_mem);
err_kmalloc:
    return -ENOMEM;
}

/* 演示内存属性和状态 */
static void show_memory_stats(struct seq_file *m)
{
    struct page *page;
    unsigned long pfn;

    seq_printf(m, "Memory Management Demo Statistics:\n\n");

    /* 显示系统内存信息 */
    seq_printf(m, "System Memory Information:\n");
    seq_printf(m, "Page Size: %lu bytes\n", PAGE_SIZE);
    seq_printf(m, "Total RAM: %lu MB\n", totalram_pages() * PAGE_SIZE / 1024 / 1024);
    seq_printf(m, "Free RAM: %lu MB\n", si_mem_available() * PAGE_SIZE / 1024 / 1024);

    /* 显示已分配内存的信息 */
    seq_printf(m, "\nAllocated Memory Information:\n");
    if (kmalloc_mem) {
        seq_printf(m, "kmalloc memory at 0x%px\n", kmalloc_mem);
        pfn = virt_to_phys(kmalloc_mem) >> PAGE_SHIFT;
        page = pfn_to_page(pfn);
        seq_printf(m, "  Page flags: 0x%lx\n", page->flags);
    }

    if (vmalloc_mem) {
        seq_printf(m, "vmalloc memory at 0x%px\n", vmalloc_mem);
        /* vmalloc内存可能不连续，这里只显示第一个页面 */
        page = vmalloc_to_page(vmalloc_mem);
        if (page)
            seq_printf(m, "  First page flags: 0x%lx\n", page->flags);
    }

    if (alloc_pages_mem) {
        seq_printf(m, "alloc_pages memory at page 0x%px\n", alloc_pages_mem);
        seq_printf(m, "  Page flags: 0x%lx\n", alloc_pages_mem->flags);
        seq_printf(m, "  Order: %d (size: %lu bytes)\n", 
                  HIGH_ORDER, PAGE_SIZE << HIGH_ORDER);
    }

    if (dma_mem) {
        seq_printf(m, "DMA memory at 0x%px (bus addr: 0x%llx)\n",
                  dma_mem, (unsigned long long)dma_handle);
    }
}

/* procfs接口实现 */
static int stats_show(struct seq_file *m, void *v)
{
    show_memory_stats(m);
    return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, stats_show, NULL);
}

static ssize_t control_write(struct file *file, const char __user *ubuf,
                           size_t count, loff_t *ppos)
{
    char buf[32];
    size_t buf_size;
    unsigned long addr;

    buf_size = min(count, sizeof(buf) - 1);
    if (copy_from_user(buf, ubuf, buf_size))
        return -EFAULT;
    buf[buf_size] = '\0';

    if (strncmp(buf, "walk ", 5) == 0) {
        if (kstrtoul(buf + 5, 16, &addr) == 0) {
            walk_page_table(addr);
        }
    }

    return count;
}

static const struct proc_ops stats_ops = {
    .proc_open = stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops control_ops = {
    .proc_write = control_write,
};

static int __init memory_demo_init(void)
{
    int ret;

    /* 创建procfs目录和文件 */
    demo_dir = proc_mkdir("memory_demo", NULL);
    if (!demo_dir)
        return -ENOMEM;

    stats_entry = proc_create("stats", 0444, demo_dir, &stats_ops);
    if (!stats_entry) {
        ret = -ENOMEM;
        goto err_stats;
    }

    control_entry = proc_create("control", 0220, demo_dir, &control_ops);
    if (!control_entry) {
        ret = -ENOMEM;
        goto err_control;
    }

    /* 分配演示用的内存 */
    ret = allocate_memory();
    if (ret)
        goto err_alloc;

    pr_info("Memory management demo module loaded\n");
    return 0;

err_alloc:
    proc_remove(control_entry);
err_control:
    proc_remove(stats_entry);
err_stats:
    proc_remove(demo_dir);
    return ret;
}

static void __exit memory_demo_exit(void)
{
    /* 释放所有分配的内存 */
    if (dma_mem)
        dma_free_coherent(NULL, DEMO_SIZE, dma_mem, dma_handle);

    if (PageHighMem(alloc_pages_mem) && high_mem)
        kunmap(alloc_pages_mem);

    if (alloc_pages_mem)
        __free_pages(alloc_pages_mem, HIGH_ORDER);

    if (vmalloc_mem)
        vfree(vmalloc_mem);

    if (kmalloc_mem)
        kfree(kmalloc_mem);

    /* 移除procfs入口 */
    proc_remove(control_entry);
    proc_remove(stats_entry);
    proc_remove(demo_dir);

    pr_info("Memory management demo module unloaded\n");
}

module_init(memory_demo_init);
module_exit(memory_demo_exit); 