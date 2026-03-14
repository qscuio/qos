#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include "lookup_symbol.h"

#if defined __i386__
#define ASM_HOOK_CODE "\x68\x00\x00\x00\x00\xc3" // push 0x00000000, ret
#define ASM_HOOK_CODE_OFFSET 1
#elif defined __x86_64__
#define ASM_HOOK_CODE "\x48\xb8\x00\x00\x00\x00\x00\x00\x00\x00\xff\xe0" // mov rax 0x0000000000000000, jmp rax
#define ASM_HOOK_CODE_OFFSET 2
#else
#error ARCH_ERROR_MESSAGE
#endif

static char original_function_code_backup[sizeof(ASM_HOOK_CODE) - 1];
static void *devmem_is_allowed_ptr = NULL;

static int modified_devmem_function(unsigned long pagenr)
{
    return 1;
}

static int __init devmem_full_access_init(void)
{
    unsigned long addr;
    
    addr = lookup_name("devmem_is_allowed");
    if (!addr) {
        pr_err("Failed to find devmem_is_allowed symbol\n");
        return -EINVAL;
    }
    devmem_is_allowed_ptr = (void *)addr;

    preempt_disable();
    write_cr0(read_cr0() & (~0x10000)); //disable write-protected memory

    memcpy(original_function_code_backup, devmem_is_allowed_ptr, sizeof(ASM_HOOK_CODE) - 1);
    memcpy(devmem_is_allowed_ptr, ASM_HOOK_CODE, sizeof(ASM_HOOK_CODE) - 1);
    *(void **)&((char *)devmem_is_allowed_ptr)[ASM_HOOK_CODE_OFFSET] = modified_devmem_function;

    write_cr0(read_cr0() | 0x10000); // enable write-protected memory
    preempt_enable();

    printk(KERN_INFO "Memory access module loaded\n");
    return 0;
}

static void __exit cleanup_devmem_module(void)
{
    preempt_disable();
    write_cr0(read_cr0() & (~0x10000)); //disable write-protected memory

    memcpy(devmem_is_allowed_ptr, original_function_code_backup, sizeof(ASM_HOOK_CODE) - 1);

    write_cr0(read_cr0() | 0x10000); // enable write-protected memory
    preempt_enable();

    printk(KERN_INFO "Memory access module unloaded\n");
}

module_init(devmem_full_access_init);
module_exit(cleanup_devmem_module);
MODULE_AUTHOR("Ozgun Ayaz");
MODULE_DESCRIPTION("make /dev/mem writable beyond the first 1 MB");
MODULE_LICENSE("GPL");
