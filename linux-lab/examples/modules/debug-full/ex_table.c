#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
 
static void blatant_div_by_zero(void)
{
        int q, d;

        d = 0;
        asm volatile ("movl $20, %%eax;"
                "movl $0, %%edx;"
                "1: div %1;"
                "movl %%eax, %0;"
                "2:\t\n"
                "\t.section .fixup,\"ax\"\n"
                "3:\tmov\t$-1, %0\n"
                "\tjmp\t2b\n"
                "\t.previous\n"
                _ASM_EXTABLE(1b, 3b)
                : "=r"(q)
                : "b"(d)
                :"%eax"
        );

        pr_debug("q = %d\n", q);
}

static int __init hello_init(void)
{
        blatant_div_by_zero();

        return 0;
}
 
static void __exit hello_exit(void)
{
    pr_info("hello_exit\n");
}
 
module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Okash Khawaja");
MODULE_DESCRIPTION("hello world");
