#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_VERSION("0.01");
MODULE_DESCRIPTION("A simple kernel module");

static int __init hello_init(void) {
    printk(KERN_INFO "Hello, kernel module!\n");
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye, kernel module!\n");
}

module_init(hello_init);
module_exit(hello_exit);
