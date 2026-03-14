#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fdtable.h>
#include <linux/namei.h>
#include <linux/version.h>

#define FILE_PATH "/tmp/file_ops_example.txt"
#define OLD_CONTENT "OLD content to write to file.\n"
#define NEW_CONTENT "New content to write to file.\n"
#define BUFFER_SIZE 128

static int __init file_ops_init(void)
{
    struct file *filp;
#ifdef set_fs
    mm_segment_t oldfs;
#endif
    loff_t pos;
    char *buf;
    ssize_t read_len, write_len;
    int error;

    printk(KERN_INFO "file_ops_example: Initializing module\n");

    // Allocate buffer for file content
    buf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!buf) {
        printk(KERN_ALERT "file_ops_example: Failed to allocate buffer\n");
        return -ENOMEM;
    }

#ifdef set_fs
    // Set memory segment to kernel's address space
    oldfs = get_fs();
    set_fs(KERNEL_DS);
#endif

    // Create the file if it does not exist
    error = vfs_stat(FILE_PATH, &(struct kstat){});
    if (error) {
        filp = filp_open(FILE_PATH, O_RDWR | O_CREAT, 0644);
        if (IS_ERR(filp)) {
            printk(KERN_ALERT "file_ops_example: Failed to create file\n");
#ifdef set_fs
            set_fs(oldfs);
#endif
            kfree(buf);
            return PTR_ERR(filp);
        }
        printk(KERN_INFO "file_ops_example: File created\n");

	pos = 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 14, 0)
	write_len = kernel_write(filp, OLD_CONTENT, strlen(OLD_CONTENT), &pos);
#else
	write_len = vfs_write(filp, OLD_CONTENT, strlen(OLD_CONTENT), &pos);
#endif
	if (write_len < 0) {
	    printk(KERN_ALERT "file_ops_example: Failed to write to file\n");
	    filp_close(filp, NULL);
#ifdef set_fs
	    set_fs(oldfs);
#endif
	    kfree(buf);
	    return write_len;
	}
    } else {
        filp = filp_open(FILE_PATH, O_RDWR, 0);
        if (IS_ERR(filp)) {
            printk(KERN_ALERT "file_ops_example: Failed to open file\n");
#ifdef set_fs
            set_fs(oldfs);
#endif
            kfree(buf);
            return PTR_ERR(filp);
        }
    }

    // Read file content
    pos = 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 14, 0)
    read_len = kernel_read(filp, buf, BUFFER_SIZE - 1, &pos);
#else
    read_len = vfs_read(filp, buf, BUFFER_SIZE - 1, &pos);
#endif
    if (read_len < 0) {
        printk(KERN_ALERT "file_ops_example: Failed to read file\n");
        filp_close(filp, NULL);
#ifdef set_fs
        set_fs(oldfs);
#endif
        kfree(buf);
        return read_len;
    }

    buf[read_len] = '\0'; // Null-terminate the buffer
    printk(KERN_INFO "file_ops_example: File content:\n%s\n", buf);

    // Overwrite file with new content
    pos = 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 14, 0)
    write_len = kernel_write(filp, NEW_CONTENT, strlen(NEW_CONTENT), &pos);
#else
    write_len = vfs_write(filp, NEW_CONTENT, strlen(NEW_CONTENT), &pos);
#endif
    if (write_len < 0) {
        printk(KERN_ALERT "file_ops_example: Failed to write to file\n");
        filp_close(filp, NULL);
#ifdef set_fs
        set_fs(oldfs);
#endif
        kfree(buf);
        return write_len;
    }

    printk(KERN_INFO "file_ops_example: Successfully wrote new content to file\n");

    // Close file and restore address space
    filp_close(filp, NULL);
#ifdef set_fs
    set_fs(oldfs);
#endif
    kfree(buf);

    return 0;
}

static void __exit file_ops_exit(void)
{
    printk(KERN_INFO "file_ops_example: Module exiting\n");
}

module_init(file_ops_init);
module_exit(file_ops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("A simple example Linux module to open a file, create if it does not exist, read its content, and overwrite it.");
MODULE_VERSION("0.1");

