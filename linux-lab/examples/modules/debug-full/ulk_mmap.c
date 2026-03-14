#include <asm/io.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "ulk_mmap"
#define DEVICE_MAJOR 240
#define IOCTL_WRITE_MSG _IOW(DEVICE_MAJOR, 0, char *)

/*
 * The kernel and user-space processes share the same physical memory when mmap is used, which allows changes made in one to be visible in the other. 
 *
 * When the kernel maps a region of its memory to user space using mmap, both the kernel and the user-space application can access and modify this memory directly. Since they share the same physical memory, any changes made by the user-space application are immediately reflected in the kernel's view of that memory.
 *
 * The kernel can write to the memory region that is mapped to user space directly, just like it would write to any other memory it owns. 
 */
// sudo mknod /dev/ulk_mmap c 240 0
static char *kernel_buffer;

static int ulk_mmap_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "ulk_mmap: open\n");
    return 0;
}

static int ulk_mmap_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "ulk_mmap: release\n");
    return 0;
}

static int ulk_mmap_mmap(struct file *file, struct vm_area_struct *vma) {
    int ret;
    unsigned long pfn;
    unsigned long length = vma->vm_end - vma->vm_start;

    if (length > PAGE_SIZE) {
        printk(KERN_ERR "ulk_mmap: request length exceeds buffer size\n");
        return -EINVAL;
    }

    pfn = virt_to_phys((void *)kernel_buffer) >> PAGE_SHIFT;
    ret = remap_pfn_range(vma, vma->vm_start, pfn, length, vma->vm_page_prot);
    if (ret != 0) {
        printk(KERN_ERR "ulk_mmap: remap_pfn_range failed\n");
        return -EAGAIN;
    }

    printk(KERN_INFO "ulk_mmap: mmap successful\n");
    return 0;
}

static long ulk_mmap_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    char msg[128];
    char *kernel_msg = "Hello from kernel, now you can see me";

    switch (cmd) {
        case IOCTL_WRITE_MSG:
            printk(KERN_INFO "ulk_mmap: current conent %s\n", kernel_buffer);
            if (copy_from_user(msg, (char *)arg, sizeof(msg))) {
                return -EFAULT;
            }
            strncpy(kernel_buffer, msg, PAGE_SIZE);
            printk(KERN_INFO "ulk_mmap: message %s written to buffer\n", kernel_buffer);

	    set_current_state(TASK_INTERRUPTIBLE);
	    schedule_timeout(2 *HZ);
	    snprintf(kernel_buffer, strlen(kernel_msg), "%s", kernel_msg);
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static const struct file_operations ulk_mmap_fops = {
    .owner = THIS_MODULE,
    .open = ulk_mmap_open,
    .release = ulk_mmap_release,
    .mmap = ulk_mmap_mmap,
    .unlocked_ioctl = ulk_mmap_ioctl,
};

static int __init ulk_mmap_init(void) {
    int ret;

    ret = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &ulk_mmap_fops);
    if (ret < 0) {
        printk(KERN_ERR "ulk_mmap: unable to register character device\n");
        return ret;
    }

    kernel_buffer = (char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!kernel_buffer) {
        unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
        printk(KERN_ERR "ulk_mmap: unable to allocate kernel buffer\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "ulk_mmap: module loaded\n");
    return 0;
}

static void __exit ulk_mmap_exit(void) {
    kfree(kernel_buffer);
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
    printk(KERN_INFO "ulk_mmap: module unloaded\n");
}

module_init(ulk_mmap_init);
module_exit(ulk_mmap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("sample code show the usage of mmap");

