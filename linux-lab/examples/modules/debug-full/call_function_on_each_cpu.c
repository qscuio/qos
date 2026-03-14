#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/smp.h>

static struct timer_list my_timer;

static void my_function_on_cpu(void *info) {
    printk("Function executed on CPU %d\n", smp_processor_id());
}

static void my_timer_callback(struct timer_list *t) {
    printk("Timer fired on CPU %d, calling function on all CPUs\n", smp_processor_id());

    // 在所有 CPU 上执行 my_function_on_cpu
    on_each_cpu(my_function_on_cpu, NULL, 1);

    // 重新启动定时器
    mod_timer(&my_timer, jiffies + HZ); // 1 秒后触发
}

static void setup_timer_on_all_cpus(void) {
    // 初始化定时器
    timer_setup(&my_timer, my_timer_callback, 0);

    // 启动定时器，1 秒后触发
    mod_timer(&my_timer, jiffies + HZ);
}

static int __init function_on_each_cpu_init(void)
{

	setup_timer_on_all_cpus();
	return 0;
}

static void __exit function_on_each_cpu_exit(void)
{
	return;
}

module_init(function_on_each_cpu_init);
module_exit(function_on_each_cpu_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("timer/workqueue on each cpu");
MODULE_VERSION("1.0");
