#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/mm.h>

static char* msg_ptr;

/* IOCTL commands */
#define IOCTL_PATCH 0x00000001
#define IOCTL_FIX 0x00000004

/* Pointer to the system call table */
unsigned long *sys_call_table;

/* We will set this variable to 1 in our open handler and reset it
   back to zero in release handler */
int in_use = 0;
int is_set = 0;

/* Pointer to the original system call functions */
asmlinkage long (*real_open)(const char __user *filename, int flags, umode_t mode);
asmlinkage ssize_t (*real_read)(unsigned int fd, char __user *buf, size_t count);
asmlinkage ssize_t (*real_write)(unsigned int fd, const char __user *buf, size_t count);
asmlinkage int (*real_close)(unsigned int fd);

/* Custom system call implementations */
static asmlinkage long custom_open(const char __user *filename, int flags, umode_t mode)
{
    printk(KERN_INFO "SysCallInterceptModule: open(%s, %d, %d) system call invoked\n", filename, flags, mode);
    return real_open(filename, flags, mode);
}

static asmlinkage ssize_t custom_read(unsigned int fd, char __user *buf, size_t count)
{
    ssize_t ret = 0;

    if (fd != 1 && fd != 2)
        printk(KERN_INFO "SysCallInterceptModule: read(%d) system call invoked\n", fd);

    ret = real_read(fd, buf, count);
    if (buf)
        printk(KERN_INFO "SysCallInterceptModule: read buffer: %s\n", buf);

    return ret;
}

static asmlinkage ssize_t custom_write(unsigned int fd, const char __user *buf, size_t count)
{
    if (fd != 1 && fd != 2)
        printk(KERN_INFO "SysCallInterceptModule: write(%d) system call invoked\n", fd);
    return real_write(fd, buf, count);
}

static asmlinkage int custom_close(unsigned int fd)
{
    printk(KERN_INFO "SysCallInterceptModule: close(%d) system call invoked\n", fd);
    return real_close(fd);
}

/* Make page writable */
static int make_writable(unsigned long address)
{
    unsigned int level;
    pte_t *pte = lookup_address(address, &level);
    if (pte->pte &~ _PAGE_RW)
        pte->pte |= _PAGE_RW;
    return 0;
}

/* Make page read-only */
static int make_readonly(unsigned long address)
{
    unsigned int level;
    pte_t *pte = lookup_address(address, &level);
    pte->pte = pte->pte &~ _PAGE_RW;
    return 0;
}

/* Device operations */
static int device_open(struct inode *inode, struct file *file)
{
    if (in_use)
        return -EBUSY;
    in_use++;
    printk(KERN_INFO "SysCallInterceptModule: device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    in_use--;
    printk(KERN_INFO "SysCallInterceptModule: device closed\n");
    return 0;
}

static ssize_t device_read(struct file* filep, char* buffer, size_t len, loff_t *offset)
{
    printk(KERN_INFO "SysCallInterceptModule: device read\n");
    return 0;
}

static ssize_t device_write(struct file* filep, const char* buff, size_t len, loff_t *off)
{
    printk(KERN_INFO "SysCallInterceptModule: device write\n");
    if (copy_from_user(msg_ptr, buff, len))
        return -EFAULT;
    if (*msg_ptr)
        printk(KERN_INFO "SysCallInterceptModule: write data: %s\n", msg_ptr);
    return len;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    switch (cmd)
    {
        case IOCTL_PATCH:
            make_writable((unsigned long)sys_call_table);

            real_open = (void*)sys_call_table[__NR_open];
            sys_call_table[__NR_open] = (unsigned long)custom_open;

            real_read = (void*)sys_call_table[__NR_read];
            sys_call_table[__NR_read] = (unsigned long)custom_read;

            real_write = (void*)sys_call_table[__NR_write];
            sys_call_table[__NR_write] = (unsigned long)custom_write;

            real_close = (void*)sys_call_table[__NR_close];
            sys_call_table[__NR_close] = (unsigned long)custom_close;

            make_readonly((unsigned long)sys_call_table);
            is_set = 1;
            break;

        case IOCTL_FIX:
            make_writable((unsigned long)sys_call_table);

            sys_call_table[__NR_open] = (unsigned long)real_open;
            sys_call_table[__NR_read] = (unsigned long)real_read;
            sys_call_table[__NR_write] = (unsigned long)real_write;
            sys_call_table[__NR_close] = (unsigned long)real_close;

            make_readonly((unsigned long)sys_call_table);
            is_set = 0;
            break;

        default:
            printk(KERN_INFO "SysCallInterceptModule: invalid IOCTL command\n");
            retval = -EINVAL;
            break;
    }
    return retval;
}

static const struct file_operations device_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static struct miscdevice miscchar_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "MiscCharDevice",
    .fops = &device_fops,
};

static int __init initModule(void)
{
    int retval;
    printk(KERN_INFO "SysCallInterceptModule: loaded\n");

    sys_call_table = (unsigned long*)kallsyms_lookup_name("sys_call_table");
    if (!sys_call_table) {
        printk(KERN_ERR "SysCallInterceptModule: unable to locate sys_call_table\n");
        return -EFAULT;
    }

    retval = misc_register(&miscchar_device);
    return retval;
}

static void __exit exitModule(void)
{
    printk(KERN_INFO "SysCallInterceptModule: unloading\n");
    misc_deregister(&miscchar_device);
    if (is_set)
    {
        make_writable((unsigned long)sys_call_table);
        sys_call_table[__NR_open] = (unsigned long)real_open;
        sys_call_table[__NR_read] = (unsigned long)real_read;
        sys_call_table[__NR_write] = (unsigned long)real_write;
        sys_call_table[__NR_close] = (unsigned long)real_close;
        make_readonly((unsigned long)sys_call_table);
    }
}

module_init(initModule);
module_exit(exitModule);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DeveloperName");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("This module demonstrates how to intercept system calls in Linux");

