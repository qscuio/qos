#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "chardev_example"
#define BUF_LEN 1024

MODULE_LICENSE("GPL");

// After load the module, run the following command.
//
// sudo mknod /dev/chardev_example c 248 0

static int major_num;
static char device_buffer[BUF_LEN];
static int device_open_count = 0;

// Function prototypes
static int chardev_open(struct inode *, struct file *);
static int chardev_release(struct inode *, struct file *);
static ssize_t chardev_read(struct file *, char *, size_t, loff_t *);
static ssize_t chardev_write(struct file *, const char *, size_t, loff_t *);

// File operations structure
static struct file_operations fops = {
    .open = chardev_open,
    .read = chardev_read,
    .write = chardev_write,
    .release = chardev_release,
};

// Module initialization
static int __init chardev_init(void) {
    // Register the character device
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ALERT "Failed to register a major number\n");
        return major_num;
    }
    printk(KERN_INFO "Registered correctly with major number %d\n", major_num);
    return 0;
}

// Module cleanup
static void __exit chardev_exit(void) {
    // Unregister the character device
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "Goodbye, World!\n");
}

// Called when a process tries to open the device file
static int chardev_open(struct inode *inodep, struct file *filep) {
    device_open_count++;
    printk(KERN_INFO "Device has been opened %d time(s)\n", device_open_count);
    return 0;
}

// Called when a process closes the device file
static int chardev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Device successfully closed\n");
    return 0;
}

// Called when a process reads from the device file
static ssize_t chardev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int bytes_read = 0;
    // Transfer data from kernel space to user space
    if (*offset >= BUF_LEN) {
        return 0; // End of file
    }
    if (*offset + len > BUF_LEN) {
        len = BUF_LEN - *offset;
    }
    if (copy_to_user(buffer, device_buffer + *offset, len) != 0) {
        return -EFAULT;
    }
    *offset += len;
    bytes_read = len;
    printk(KERN_INFO "Read %d bytes from device\n", bytes_read);
    return bytes_read;
}

// Called when a process writes to the device file
static ssize_t chardev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    int bytes_written = 0;
    // Transfer data from user space to kernel space
    if (*offset >= BUF_LEN) {
        return -ENOSPC; // No space left on device
    }
    if (*offset + len > BUF_LEN) {
        len = BUF_LEN - *offset;
    }
    if (copy_from_user(device_buffer + *offset, buffer, len) != 0) {
        return -EFAULT;
    }
    *offset += len;
    bytes_written = len;
    printk(KERN_INFO "Wrote %d bytes to device\n", bytes_written);
    return bytes_written;
}

module_init(chardev_init);
module_exit(chardev_exit);

