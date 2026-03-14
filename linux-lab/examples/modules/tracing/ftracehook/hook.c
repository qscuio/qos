#include "hook.h"

int start_hook(fthook_t *hook, uintptr_t hooked_function, ftrace_fn hook_function)
{
    int ret = 0;

    hook->active = 0;
    hook->original_function = hooked_function;
    hook->ops.func = hook_function;
    hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS
	| FTRACE_OPS_FL_RECURSION
	| FTRACE_OPS_FL_IPMODIFY;

    ret = ftrace_set_filter_ip(&hook->ops, hook->original_function, 0, 0);

    if (ret) {
	return 1;
    }

    ret = register_ftrace_function(&hook->ops);

    if (ret) {
	ftrace_set_filter_ip(&hook->ops, hook->original_function, 1, 0);
	return 2;
    }

    hook->active = 1;

    return 0;
}

int end_hook(fthook_t *hook)
{
    int ret = unregister_ftrace_function(&hook->ops);

    hook->active = 0;

    if (ret)
	return ret;

    ftrace_set_filter_ip(&hook->ops, hook->original_function, 1, 0);

    return 0;
}

#include <linux/kprobes.h>
static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};
typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

int start_hook_list(const fthinit_t *hook_list, size_t size)
{
    size_t i;
    uintptr_t symaddr;
    int ret = 0;

    kallsyms_lookup_name_t kallsyms_lookup_name;
    register_kprobe(&kp);
    kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
    unregister_kprobe(&kp);

    for (i = 0; i < size; i++) {
	if (hook_list[i].symbol_name) {
	    symaddr = kallsyms_lookup_name(hook_list[i].symbol_name);
	} else {
	    symaddr = hook_list[i].symbol_getter();
	}

	if (!symaddr) {
	    continue;
	}

	ret = start_hook(hook_list[i].hook, symaddr, hook_list[i].hook_function);

	if (ret) {
	    end_hook_list(hook_list, i);
	    return ret;
	}
    }

    return 0;
}

int end_hook_list(const fthinit_t *hook_list, size_t size)
{
    size_t i;
    int ret = 0, ret2 = 0;

    for (i = 0; i < size; i++) {
	if (hook_list[i].hook->active)
	    ret2 = end_hook(hook_list[i].hook);

	if (ret2)
	    ret = ret2;
    }

    return ret;
}

void kp_print(void * addr, unsigned long size)
{
    int i;
    char * t = (char *)addr;

    printk(KERN_INFO "Dumping memory at %pK:\n", addr);

    for(i = 0 ; i < KP_PRINT_COLUMNS; i++) {
	if(i % KP_PRINT_COLUMNS == 0) {
	    printk(KERN_CONT "       ");
	}

	printk(KERN_CONT "%02d ", i);
    }

    printk(KERN_CONT "\n\n");

    for(i = 0 ; i < size; i++) {
	if(i % KP_PRINT_COLUMNS == 0) {
	    if(i) {
		printk(KERN_CONT "\n");
	    }

	    printk("%04x   ", i / KP_PRINT_COLUMNS);
	}

	printk(KERN_CONT "%02x ", (unsigned char)t[i]);
    }

    printk(KERN_CONT "\n");
}
