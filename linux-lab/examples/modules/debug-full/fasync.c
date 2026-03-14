#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/slab.h>

#define DEVICE_NAME "fasync_example"
#define BUFFER_SIZE 1024

static int major_number;
static struct cdev my_cdev;
static char *device_buffer;
static int buffer_head, buffer_tail;
static struct fasync_struct *async_queue;

static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t device_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static int device_fasync(int fd, struct file *file, int mode);

static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "fasync_example: device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    device_fasync(-1, file, 0);
    printk(KERN_INFO "fasync_example: device closed\n");
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    int bytes_read = 0;
    if (buffer_tail == buffer_head) {
        return 0; // No data to read
    }

    while (count && buffer_tail != buffer_head) {
        put_user(device_buffer[buffer_tail], buf++);
        buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
        count--;
        bytes_read++;
    }

    return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    int bytes_written = 0;

    while (count && ((buffer_head + 1) % BUFFER_SIZE != buffer_tail)) {
        get_user(device_buffer[buffer_head], buf++);
        buffer_head = (buffer_head + 1) % BUFFER_SIZE;
        count--;
        bytes_written++;
    }

    if (async_queue) {
        kill_fasync(&async_queue, SIGIO, POLL_IN);
    }

    return bytes_written;
}

static int device_fasync(int fd, struct file *file, int mode) {
    return fasync_helper(fd, file, mode, &async_queue);
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
    .fasync = device_fasync,
};

static int __init my_module_init(void) {
    int result;
    dev_t dev;

    result = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (result < 0) {
        printk(KERN_ALERT "Failed to allocate character device region\n");
        return result;
    }

    major_number = MAJOR(dev);
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    result = cdev_add(&my_cdev, dev, 1);
    if (result < 0) {
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "Failed to add character device\n");
        return result;
    }

    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "Failed to allocate device buffer\n");
        return -ENOMEM;
    }

    buffer_head = buffer_tail = 0;
    async_queue = NULL;

    printk(KERN_INFO "fasync_example: module loaded\n");
    return 0;
}

static void __exit my_module_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(MKDEV(major_number, 0), 1);
    kfree(device_buffer);
    printk(KERN_INFO "fasync_example: module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Example module using fasync");

