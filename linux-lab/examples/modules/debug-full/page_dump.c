#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include "lookup_symbol.h"

static void (*show_mem_ptr)(unsigned int flags, nodemask_t *nodemask);

static int __init page_dump_init(void)
{
    unsigned long addr;
    
    addr = lookup_name("__show_mem");
    if (!addr) {
        pr_err("Failed to find __show_mem symbol\n");
        return -EINVAL;
    }
    show_mem_ptr = (void *)addr;
    
    // Call show_mem with no flags and NULL nodemask
    show_mem_ptr(0, NULL);
    return 0;
}

static void __exit page_dump_exit(void)
{
}

module_init(page_dump_init);
module_exit(page_dump_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to dump physical pages");

