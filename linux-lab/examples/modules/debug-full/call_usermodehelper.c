#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>

static int __init call_user_init(void)
{
    char *argv[] = { "/usr/bin/ping", "10.10.10.254", NULL };
    char *envp[] = { "HOME=/home/qwert", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
    int ret;

    printk(KERN_INFO "Call User: Ping to 10.10.10.254 \n");
    ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    if (ret)
        printk(KERN_ERR "Call User: Helper execution failed with code %d\n", ret);
    else
        printk(KERN_INFO "Call User: Helper executed successfully\n");

    return ret;
}

static void __exit call_user_exit(void)
{
    printk(KERN_INFO "Call User: Exiting\n");
}

module_init(call_user_init);
module_exit(call_user_exit);
MODULE_LICENSE("GPL");
