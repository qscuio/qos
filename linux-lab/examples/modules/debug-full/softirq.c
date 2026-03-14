#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>

/* We share the interrup of eth01/eth02 to simulate interrupt source. */
#define LOCAL_IRQ           11    /* eth01 */
#define LOCAL_HI_IRQ        10    /* eth02 */

static struct task_struct *kthread_lo;
static struct task_struct *kthread_hi;

static void qulk_lo_softirq_handler(struct softirq_action *a);
static void qulk_hi_softirq_handler(struct softirq_action *a);

static struct softirq_action qulk_hi_softirq = 
{
    .action = qulk_hi_softirq_handler,
};

static struct softirq_action qulk_lo_softirq = 
{
    .action = qulk_lo_softirq_handler,
};

#define __TRACE() do                                                                                                     \
{                                                                                                                        \
    pr_info("[%s][%d] in_irq %ld in_softirq %ld in_interrupt %ld preempt_count %d preemptible %d cpu %d\n",              \
	    __func__, __LINE__, in_irq(), in_softirq(), in_interrupt(), preempt_count(), preemptible(), smp_processor_id()); \
} while(0)


static int kthread_lo_fn(void *data)
{
    while(!kthread_should_stop())
    {
	__TRACE();
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
    }

    return 0;
}

static int kthread_hi_fn(void *data)
{
    while(!kthread_should_stop())
    {
	__TRACE();
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
    }

    return 0;
}

static irqreturn_t qulk_lo_irq_handler(int irq, void *devid)
{
    __TRACE();
    raise_softirq(ULK_SOFTIRQ);
    return IRQ_HANDLED;
}

static irqreturn_t qulk_hi_irq_handler(int irq, void *devid)
{
    __TRACE();
    raise_softirq(ULK_HI_SOFTIRQ);
    return IRQ_HANDLED;
}

static void qulk_lo_softirq_handler(struct softirq_action *a)
{
    __TRACE();
    wake_up_process(kthread_lo);
    return;
}

static void qulk_hi_softirq_handler(struct softirq_action *a)
{
    __TRACE();
    wake_up_process(kthread_hi);
    return;
}

static int __init qulk_irq_init(void) 
{
    int ret = 0;

    pr_info("qulk irq module inserted\n");

    open_softirq(ULK_SOFTIRQ, qulk_lo_softirq_handler);
    open_softirq(ULK_HI_SOFTIRQ, qulk_hi_softirq_handler);

    ret = request_irq(LOCAL_IRQ, qulk_lo_irq_handler, IRQF_SHARED, "low_softirq_trigger", &qulk_lo_softirq);
    if (ret) 
    {
	pr_err("Failed to request softirq\n");
	return ret;
    }

    ret = request_irq(LOCAL_HI_IRQ, qulk_hi_irq_handler, IRQF_SHARED, "high_softirq_trigger", &qulk_hi_softirq);
    if (ret) 
    {
	pr_err("Failed to request softirq\n");
	return ret;
    }

    kthread_lo = kthread_create(&kthread_lo_fn, NULL, "klof");
    kthread_hi = kthread_create(&kthread_hi_fn, NULL, "khif");
    wake_up_process(kthread_lo);
    wake_up_process(kthread_hi);

    return 0;
}

static void __exit qulk_irq_exit(void) 
{
    kthread_stop(kthread_lo);
    kthread_stop(kthread_hi);

    open_softirq(ULK_SOFTIRQ, NULL);
    open_softirq(ULK_HI_SOFTIRQ, NULL);

    free_irq(LOCAL_IRQ, &qulk_lo_softirq);
    free_irq(LOCAL_HI_IRQ, &qulk_hi_softirq);

    pr_info("qulk irq module exited\n");
}

module_init(qulk_irq_init);
module_exit(qulk_irq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("Check Linux kernel irq/soft_irq api");
MODULE_VERSION("0.1");
