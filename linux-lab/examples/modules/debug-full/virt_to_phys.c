#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <asm/page.h>

// Function to get the physical address from a virtual address
static void print_virtual_to_physical(unsigned long virt_addr) {
    unsigned long phys_addr;
    phys_addr = virt_to_phys((volatile void *)virt_addr);
    printk(KERN_INFO "Virtual address: 0x%lx, Physical address: 0x%lx\n", virt_addr, phys_addr);
}

// Kernel module initialization
static int __init mymodule_init(void) {
    unsigned long test_virt_addr = __get_free_page(GFP_KERNEL);
    if (!test_virt_addr) {
        printk(KERN_ERR "Failed to allocate test page\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "Allocated test page at virtual address: 0x%lx\n", test_virt_addr);
    print_virtual_to_physical(test_virt_addr);

    free_page(test_virt_addr);
    return 0;
}

// Kernel module cleanup
static void __exit mymodule_exit(void) {
    printk(KERN_INFO "Module unloaded\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to print virtual to physical address mapping");

