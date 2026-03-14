#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/security.h>
#include <linux/memblock.h>
#include <linux/kprobes.h>
#include "lookup_symbol.h"

// cat /sys/kernel/debug/init_mm_dump/dump 
//
static struct dentry *dir, *file;

static struct mm_struct *init_mm_ptr;

static int dump_init_mm_show(struct seq_file *m, void *v)
{
    seq_printf(m, "pgd: %px\n", init_mm_ptr->pgd);
    seq_printf(m, "mmap_base: %lx\n", init_mm_ptr->mmap_base);

    return 0;
}

static int dump_init_mm_open(struct inode *inode, struct file *file)
{
    return single_open(file, dump_init_mm_show, NULL);
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dump_init_mm_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init dump_init_mm_init(void)
{
    unsigned long addr;
    
    addr = lookup_name("init_mm");
    if (!addr) {
        pr_err("Failed to find init_mm symbol\n");
        return -EINVAL;
    }
    init_mm_ptr = (struct mm_struct *)addr;
    
    dir = debugfs_create_dir("init_mm_dump", NULL);
    if (!dir)
        return -ENOMEM;

    file = debugfs_create_file("dump", 0444, dir, NULL, &fops);
    if (!file) {
        debugfs_remove(dir);
        return -ENOMEM;
    }

    pr_info("init_mm dump module loaded\n");
    return 0;
}

static void __exit dump_init_mm_exit(void)
{
    debugfs_remove_recursive(dir);
    pr_info("init_mm dump module unloaded\n");
}

module_init(dump_init_mm_init);
module_exit(dump_init_mm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A module to dump init_mm structure using DebugFS");

