#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/version.h>

#define DEVICE_NAME "vm_ops_example"
#define CLASS_NAME  "vmops"

#define __TRACE() do                                                                                                     \
{                                                                                                                        \
    pr_info("[%s][%d] in_irq %ld in_softirq %ld in_interrupt %ld preempt_count %d preemptible %d cpu %d\n",              \
	    __func__, __LINE__, in_irq(), in_softirq(), in_interrupt(), preempt_count(), preemptible(), smp_processor_id()); \
    dump_stack();                                                                                                            \
} while(0)

static int major_number;
static struct class *vmops_class = NULL;
static struct device *vmops_device = NULL;

struct vm_data {
    char *data;
    char name[64];
    size_t size;
};

static void vm_open(struct vm_area_struct *vma)
{
    __TRACE();
}

static void vm_close(struct vm_area_struct *vma)
{
    __TRACE();
}

static vm_fault_t vm_fault(struct vm_fault *vmf)
{
    struct page *page;
    struct vm_data *data = (struct vm_data *)vmf->vma->vm_private_data;

    __TRACE();

    if (!data || !data->data) {
        return VM_FAULT_SIGBUS;
    }

    page = virt_to_page(data->data);
    get_page(page);
    vmf->page = page;
    return 0;
}

static const char *vm_name(struct vm_area_struct *vma)
{
    struct vm_data *data = vma->vm_private_data;

    __TRACE();

    return data->name;
}

static struct vm_operations_struct vm_ops = {
    .open = vm_open,
    .close = vm_close,
    .fault = vm_fault,
    .name = vm_name,
};

static int device_open(struct inode *inode, struct file *file)
{
#if 0
    struct path *path_struct;
#endif
    struct vm_data *data;

    __TRACE();

    data = kmalloc(sizeof(struct vm_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->size = PAGE_SIZE;
    data->data = (char *)get_zeroed_page(GFP_KERNEL);

    if (!data->data) {
        kfree(data);
        return -ENOMEM;
    }

#if 0
    path_struct = &file->f_path;
    path_get(path_struct);

    d_path(path_struct, data->name, PATH_MAX);

    path_put(path_struct);
#else
    file_path(file, data->name, PATH_MAX);
#endif
    strcpy(data->data, "Hello from kernel space!");

    file->private_data = data;

    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    struct vm_data *data = file->private_data;

    __TRACE();

    if (data) {
        if (data->data)
            free_page((unsigned long)data->data);
        kfree(data);
    }

    return 0;
}

static int device_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct vm_data *data = file->private_data;

    __TRACE();
    vma->vm_ops = &vm_ops;
    vma->vm_private_data = data;
    vm_open(vma);

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .mmap = device_mmap,
};

static int __init vmops_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "vm_ops_example failed to register a major number\n");
        return major_number;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 317)
    vmops_class = class_create(THIS_MODULE, CLASS_NAME);
#else
    vmops_class = class_create(CLASS_NAME);
#endif
    if (IS_ERR(vmops_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(vmops_class);
    }

    vmops_device = device_create(vmops_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(vmops_device)) {
        class_destroy(vmops_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(vmops_device);
    }

    printk(KERN_INFO "vm_ops_example: device class created correctly\n");
    return 0;
}

static void __exit vmops_exit(void)
{
    device_destroy(vmops_class, MKDEV(major_number, 0));
    class_unregister(vmops_class);
    class_destroy(vmops_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "vm_ops_example: Goodbye from the kernel!\n");
}

module_init(vmops_init);
module_exit(vmops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("A simple example Linux module demonstrating vm_operations_struct");
MODULE_VERSION("0.1");
