#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h> // Required for copy_to_user
#include <linux/fs.h> // Required for character device operations
#include <linux/cdev.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux kernel module for memory allocation with data transfer to user space");
MODULE_VERSION("0.1");

#define DEVICE_NAME "memory_alloc"

static struct cdev memory_cdev;
static struct page *single_page;
static char *single_page_addr;
static unsigned long *multi_pages;
static char *multi_page_addr;
static char *kernel_buffer; // Buffer to store data in kernel space
static int major_number; // Major number for the device driver
static struct class *memory_class = NULL; // Device class

// Function to copy data from kernel space to user space
static ssize_t memory_alloc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    size_t to_copy = min(count, PAGE_SIZE); // Copy at most PAGE_SIZE bytes

    if (copy_to_user(buf, kernel_buffer, to_copy) != 0) {
        return -EFAULT; // Error copying data to user space
    }
    printk("SINGLE: %s\n", single_page_addr);
    printk("MULTI:  %s\n", multi_page_addr);

    return to_copy; // Return number of bytes copied
}

// Define file operations for the character device
static const struct file_operations memory_alloc_fops = {
    .owner = THIS_MODULE,
    .read = memory_alloc_read,
};

static int __init memory_alloc_init(void)
{
    int ret;
    int order = 2; // Allocate 2^order pages (i.e., 4 pages)

    printk(KERN_INFO "Memory allocation module: initializing\n");

    // Allocate a major number dynamically
    ret = alloc_chrdev_region(&major_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate major number\n");
        return ret;
    }

    printk(KERN_INFO "Create device with majore number %d\n", MAJOR(major_number));

    // Create a class for the device in sysfs
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 317)
    memory_class = class_create(THIS_MODULE, DEVICE_NAME);
#else
    memory_class = class_create(DEVICE_NAME);
#endif
    if (IS_ERR(memory_class)) {
        unregister_chrdev_region(major_number, 1);
        printk(KERN_ERR "Failed to register device class\n");
        return PTR_ERR(memory_class);
    }

    // Initialize the character device
    cdev_init(&memory_cdev, &memory_alloc_fops);
    memory_cdev.owner = THIS_MODULE;

    ret = cdev_add(&memory_cdev, major_number, 1);
    if (ret < 0) {
        class_destroy(memory_class);
        unregister_chrdev_region(major_number, 1);
        printk(KERN_ERR "Failed to add character device\n");
        return ret;
    }

    single_page = alloc_page(GFP_KERNEL);
    if (!single_page) {
        printk(KERN_ERR "Failed to allocate single page\n");
        return -ENOMEM;
    }

    single_page_addr = page_address(single_page);

    multi_pages = (unsigned long *)__get_free_pages(GFP_KERNEL, order);
    if (!multi_pages) {
        printk(KERN_ERR "Failed to allocate multiple pages\n");
        __free_page(single_page); // Free single page
        return -ENOMEM;
    }

    multi_page_addr = (char *)(multi_pages);

    kernel_buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!kernel_buffer) {
        printk(KERN_ERR "Failed to allocate kernel buffer\n");
        free_pages((unsigned long)multi_pages, order); // Free multiple pages
        __free_page(single_page); // Free single page
        return -ENOMEM;
    }

    strncpy(single_page_addr, "Hello from single page!", PAGE_SIZE - 1);
    single_page_addr[PAGE_SIZE - 1] = '\0'; // Ensure null-terminated string

    strncpy(multi_page_addr, "Hello from multi page!", PAGE_SIZE - 1);
    multi_page_addr[PAGE_SIZE * order- 1] = '\0'; // Ensure null-terminated string

    strncpy(kernel_buffer, "Hello from kmalloc!", PAGE_SIZE - 1);
    kernel_buffer[PAGE_SIZE - 1] = '\0'; // Ensure null-terminated string

    printk(KERN_INFO "Kernel buffer contains: %s\n", kernel_buffer);

    return 0;
}

static void __exit memory_alloc_exit(void)
{
    printk(KERN_INFO "Memory allocation module: exiting\n");

    cdev_del(&memory_cdev);

    class_destroy(memory_class);

    unregister_chrdev_region(major_number, 1);

    kfree(kernel_buffer);
    free_pages((unsigned long)multi_pages, 2);
    __free_page(single_page);
}

module_init(memory_alloc_init);
module_exit(memory_alloc_exit);

