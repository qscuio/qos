#include <linux/kernel.h>

void foo(void);

void foo(void)
{
	printk(KERN_INFO "Hi from Foo\n");
}
