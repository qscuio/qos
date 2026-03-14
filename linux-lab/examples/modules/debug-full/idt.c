/*Patches syscall dispatcher */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <asm/errno.h>
#include <asm/unistd.h>
#include <linux/mman.h>
#include <asm/proto.h>
#include <asm/delay.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <asm/desc.h>
#include <linux/slab.h>
 
static struct file_operations chdir_ops;
void (*syscall_handler)(void);
unsigned int real_addr,patchr;
unsigned int *idt_base;
gate_desc *orig_syscall;
 
void
patch(void){
    printk("Good Body\n");
}
void
fake_syscall_dispatcher(void){
    /* steps:
     *  1- reverse the stdcall stack frame instructions
     *  2- store the stack frame
     *  3- do [Nice] things
     *  4- restore stack frame
     *  5- call system call
     */
    __asm__ __volatile__ (
        "movl %ebp,%esp\n"
        "pop %ebp\n");
    __asm__ __volatile__ (
        ".global fake_syscall\n"
        ".align 4,0x90\n"
    );
 
    __asm__ __volatile__ (
    "fake_syscall:\n"
        "pushl %ds\n"
        "pushl %eax\n"
        "pushl %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %edx\n"
        "pushl %ecx\n"
        "pushl %ebx\n"
        "xor %ebx,%ebx\n");
 
    __asm__ __volatile__ (
        "movl $12,%ebx\n"
        "cmpl %eax,%ebx\n"
        "jne done\n"
        );
    __asm__ __volatile__(
        "\tmov %esp,%edx\n"
        "\tmov %esp, %eax\n"
        "\tpushl %eax\n"
        "\tpush %edx\n"
        );
    __asm__ __volatile__(
        "\tcall *%0\n"
        "\tpop %%ebp\n"
        "\tpop %%edx\n"
        "\tmovl %%edx,%%esp\n"
        "done:\n"
        "\tpopl %%ebx\n"
        "\tpopl %%ecx\n"
        "\tpopl %%edx\n"
        "\tpopl %%esi\n"
        "\tpopl %%edi\n"
        "\tpopl %%ebp\n"
        "\tpopl %%eax\n"
        "\tpopl %%ds\n"
        "\tjmp *%1\n"
        :: "m" (patchr), "m"(syscall_handler));
}
 
int __init
chdir_init(void){
    /** Interrupt descriptor
     *  base address of idt_table
     */
    struct desc_ptr idtr;
    unsigned int syscall_disp;
    gate_desc  *new_syscall;
 
    new_syscall = (gate_desc *)kmalloc(sizeof(gate_desc), GFP_KERNEL);
    orig_syscall = (gate_desc *)kmalloc(sizeof(gate_desc), GFP_KERNEL);
 
    store_idt(&idtr);
    idt_base = (unsigned int *)idtr.address;
 
    /* Two ways,
     * 1- extract syscall handler address from idt table
     * 2- register interrupt and hook it with syscall handler
     * METHOD 1:
     */
    patchr = (unsigned int) patch;
    *orig_syscall = ((gate_desc *) idt_base)[0x80];
 
    /* System call dispatcher address */
    syscall_disp = (orig_syscall->a & 0xFFFF) | (orig_syscall->b & 0xFFFF0000);
    *((unsigned int *) &syscall_handler) = syscall_disp;
    real_addr = syscall_disp;
 
    //construct new gate_desc for fake dispatcher
        // copy segment descriptor from original syscall dispatcher gatedesc
    new_syscall->a = (orig_syscall->a & 0xFFFF0000);
        // copy flags from the original syscall dispatcher
    new_syscall->b = (orig_syscall->b & 0x0000FFFF);
    new_syscall->a |=(unsigned int) (((unsigned int)fake_syscall_dispatcher) & 0x0000FFFF);
    new_syscall->b |=(unsigned int) (((unsigned int)fake_syscall_dispatcher) & 0xFFFF0000);
 
    printk("Old desc [a]=%x\t[b]=%x\t[addr]=%p\n",
        orig_syscall->a,orig_syscall->b,orig_syscall);
    printk("New desc [a]=%x\t[b]=%x\t[addr]=%p\n\n",
        new_syscall->a,new_syscall->b,&new_syscall);
    printk("Old desc [a]=%x\t[b]=%x\t[addr]=%p\n",
        orig_syscall->a,orig_syscall->b,((gate_desc *) idt_base)[80]);
    printk("New desc [a]=%x\t[b]=%x\t[addr]=%p\n",
        new_syscall->a,new_syscall->b,new_syscall);
    printk("Old:%p\tNew:%p\n",
        fake_syscall_dispatcher,syscall_handler);
 
    ((gate_desc *)idt_base)[0x80] = *new_syscall;
    /* Overwrite idt syscall dispatcher desc with ours */
 
    return 0;
}
 
void __exit
chdir_cleanup(void){
 
    ((gate_desc *)idt_base)[0x80] = *orig_syscall;
 
    printk("Exit\n");
    return;
}
 
static struct file_operations chdir_ops= {
    .owner  = THIS_MODULE,
};
module_init(chdir_init);
module_exit(chdir_cleanup);
MODULE_LICENSE("GPL");
