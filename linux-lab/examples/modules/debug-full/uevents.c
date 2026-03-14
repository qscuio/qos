#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux kernel module that sends uevents with kset");
MODULE_VERSION("0.1");

static struct kset *example_kset;
static struct kobject *example_kobj;

static int __init example_init(void)
{
    int ret;
    char *envp[] = { "CUSTOM_VAR=custom_value", NULL };

    printk(KERN_INFO "Example module: initializing\n");

    // Create a kset
    example_kset = kset_create_and_add("example_kset", NULL, kernel_kobj);
    if (!example_kset)
        return -ENOMEM;

    // Create a kobject and add it to the kset
    example_kobj = kobject_create_and_add("example_kobj", &example_kset->kobj);
    if (!example_kobj) {
        kset_unregister(example_kset);
        return -ENOMEM;
    }

    // Send uevent
    ret = kobject_uevent_env(example_kobj, KOBJ_ADD, envp);
    if (ret)
        printk(KERN_ERR "Example module: kobject_uevent_env failed\n");
    else
        printk(KERN_INFO "Example module: uevent sent\n");

    return ret;
}

static void __exit example_exit(void)
{
    printk(KERN_INFO "Example module: exiting\n");

    if (example_kobj)
        kobject_put(example_kobj);
    
    if (example_kset)
        kset_unregister(example_kset);
}

module_init(example_init);
module_exit(example_exit);
