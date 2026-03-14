#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include "foo.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

static int __init mf_init(void)
{
	printk(KERN_INFO "Hi from init\n");
	foo();
	return 0;
}

static __exit void mf_exit(void)
{
	printk(KERN_INFO "Bye!\n");
}

module_init(mf_init)
module_exit(mf_exit)
