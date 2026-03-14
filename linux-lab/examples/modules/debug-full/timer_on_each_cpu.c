#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/smp.h>

static DEFINE_PER_CPU(struct timer_list, per_cpu_timer);
static DEFINE_PER_CPU(struct work_struct, per_cpu_work);

static void my_work_handler(struct work_struct *work) {
    printk("Work executed on CPU %d\n", smp_processor_id());
}

static void my_timer_callback(struct timer_list *timer) {
    int cpu = smp_processor_id();
    printk("Timer fired on CPU %d\n", cpu);

    // 调度工作到当前 CPU
    schedule_work_on(cpu, &per_cpu(per_cpu_work, cpu));

    // 重新启动定时器
    mod_timer(timer, jiffies + HZ); // 每秒触发一次
}

static void setup_per_cpu_timer(int cpu) {
    struct timer_list *timer = &per_cpu(per_cpu_timer, cpu);
    struct work_struct *work = &per_cpu(per_cpu_work, cpu);

    // 初始化工作项
    INIT_WORK(work, my_work_handler);

    // 初始化定时器
    timer_setup(timer, my_timer_callback, 0);

    // 在当前 CPU 上启动定时器
    mod_timer(timer, jiffies + HZ); // 1 秒后触发
}

static int start_timers_on_each_cpu(void) {
    int cpu;

    // 遍历每个 CPU，并在其上启动定时器
    for_each_online_cpu(cpu) {
        setup_per_cpu_timer(cpu);
    }

    return 0;
}

static int __init percpu_timer_init(void)
{
	start_timers_on_each_cpu();

	return 0;
}

static void __exit percpu_timer_exit(void)
{
	return;
}

module_init(percpu_timer_init);
module_exit(percpu_timer_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("timer/workqueue on each cpu");
MODULE_VERSION("1.0");
