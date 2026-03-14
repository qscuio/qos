#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux Character Device Example");
MODULE_VERSION("0.1");

#define DEVICE_NAME "my_char_device"
#define BUF_SIZE 256

/* 
A character device in Linux is a type of device that transfers data in a stream of characters or bytes. It allows user-level applications to interact with the device using read and write operations. Below is an example of a simple Linux kernel module that creates a character device and demonstrates how to handle read and write operations on the device.
*/

static int major_number;
static char message[BUF_SIZE] = {0};
static int message_size = 0;

// Function to handle read operations on the device
static ssize_t char_device_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
    int bytes_read = 0;

    if (*offset >= message_size)
        return 0;

    if (size > message_size - *offset)
        size = message_size - *offset;

    bytes_read = size - copy_to_user(user_buffer, message + *offset, size);
    *offset += bytes_read;

    return bytes_read;
}

// Function to handle write operations on the device
static ssize_t char_device_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset) {
    int bytes_written = 0;

    if (size >= BUF_SIZE)
        return -EINVAL;

    bytes_written = size - copy_from_user(message, user_buffer, size);
    message_size = bytes_written;

    return bytes_written;
}

// File operations structure for the character device
static const struct file_operations char_device_fops = {
    .owner = THIS_MODULE,
    .read = char_device_read,
    .write = char_device_write,
};

static int __init example_module_init(void) {
    pr_info("Example module initializing\n");

    // Register the character device and obtain the major number
    major_number = register_chrdev(0, DEVICE_NAME, &char_device_fops);
    if (major_number < 0) {
        pr_err("Failed to register character device\n");
        return major_number;
    }

    pr_info("Character device registered with major number: %d\n", major_number);

    return 0;
}

static void __exit example_module_exit(void) {
    pr_info("Example module exiting\n");

    // Unregister the character device
    unregister_chrdev(major_number, DEVICE_NAME);
}

module_init(example_module_init);
module_exit(example_module_exit);

