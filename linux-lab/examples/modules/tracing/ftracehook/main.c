#include "hook.h"

#include <linux/tracepoint.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/string.h>

#include <asm/unistd.h>

MODULE_DESCRIPTION("Kernel Dynamic Hooking");
MODULE_AUTHOR("Paran Lee");
MODULE_LICENSE("GPL");

static int hook_init(void);
static void hook_exit(void);

module_init(hook_init);
module_exit(hook_exit);

#define print(format, ...) printk(KBUILD_MODNAME": "format, ##__VA_ARGS__)


// Do not 'static' with runtime crash utility!!!
int show_count = 10;
struct file *show_files[10];

DEFINE_STATIC_FUNCTION_HOOK(ssize_t, vfs_write, struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    DEFINE_ORIGINAL(vfs_write);
    if(show_count > 0 && file != NULL && file->f_op != NULL && file->f_op->write != NULL)
    {
	char buffer[64] = { '\0', };
	sprint_symbol(buffer, (unsigned long)(file->f_op->write));

	printk(KERN_CRIT "[FTRACE_HOOK] file=%p function=%ps(%lu)\n",
		file, file->f_op->write,  (unsigned long)(file->f_op->write));

	dump_stack();

	show_files[--show_count] = file;
    }

    return orig_fn(file, buf, count, pos);
}

static const fthinit_t hook_list[] = {
    HLIST_NAME_ENTRY(vfs_write)
};

static int error_quit(const char *msg)
{
    print("%s\n", msg);
    return -EFAULT;
}

static int hook_init(void)
{
    int ret;

    print("initializing...\n");

    ret = start_hook_list(hook_list, ARRAY_SIZE(hook_list));

    if (ret == -1)
	return error_quit("Last error: Failed to lookup symbols!");
    else
    {
	printk(KERN_CRIT "Last error(%d): Failed to call ftrace_set_filter_ip!", ret);
	return ret;
    }

    print("Kernel Dynamic Hooking initialized!\n");

    return 0;
}

static void hook_exit(void)
{
    int ret;
    print("unloading...\n");

    ret = end_hook_list(hook_list, ARRAY_SIZE(hook_list));

    if (ret) {
	error_quit("Failed to unregister the ftrace hook function");
	return;
    }

    print("Kernel Dynamic Hooking unloaded!\n");
}
