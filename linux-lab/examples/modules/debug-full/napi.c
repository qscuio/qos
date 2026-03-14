#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/timer.h>

#define TIMER_INTERVAL_MS_A 2000 //
#define TIMER_INTERVAL_MS_B 10 //
#define TIMER_INTERVAL_MS_C 500 //
#define TIMER_INTERVAL_MS_C_1 100 //
static struct timer_list napi_a_trigger_timer;
static struct timer_list napi_b_trigger_timer;
static struct timer_list napi_c_trigger_timer;
__maybe_unused static struct timer_list napi_c_1_trigger_timer;

static struct net_device *napi_netdev_a;
static struct net_device *napi_netdev_b;
static struct net_device *napi_netdev_c;
static struct net_device *napi_netdev_c_1;
static struct napi_struct napi_a;
static struct napi_struct napi_b;
static struct napi_struct napi_b_1;
static struct napi_struct napi_b_2;
static struct napi_struct napi_b_3;
static struct napi_struct napi_b_4;
static struct napi_struct napi_b_5;
static struct napi_struct napi_c;

static void napi_a_trigger_timer_cb(struct timer_list *timer) {
    pr_info("Timer expired! Executing timer callback\n");

    napi_schedule(&napi_a);
    mod_timer(timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_A));
}

static void napi_b_trigger_timer_cb(struct timer_list *timer) {
    pr_info("Timer expired! Executing timer callback\n");

    napi_schedule(&napi_b);
    napi_schedule(&napi_b_1);
    napi_schedule(&napi_b_2);
    napi_schedule(&napi_b_3);
    napi_schedule(&napi_b_4);
    napi_schedule(&napi_b_5);
    mod_timer(timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_B));
}

static void napi_c_trigger_timer_cb(struct timer_list *timer) {
    pr_info("Timer expired! Executing timer callback\n");

    napi_schedule(&napi_c);
    mod_timer(timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_C));
}

__maybe_unused static void napi_c_1_trigger_timer_cb(struct timer_list *timer) {
    pr_info("Timer expired! Executing timer callback\n");

    napi_schedule(&napi_c);
    mod_timer(timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_C));
}

static int napi_poll_a(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_a->irq);
#endif
    }

    return work_done;
}

static int napi_poll_b(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

static int napi_poll_b_1(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

static int napi_poll_b_2(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

static int napi_poll_b_3(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

static int napi_poll_b_4(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

static int napi_poll_b_5(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

static int napi_poll_c(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

__maybe_unused static int napi_poll_c_1(struct napi_struct *napi, int budget)
{
    int work_done = 0;

    printk(KERN_INFO "[%s] NAPI poll function called\n", __func__);

    /* Simulate some work done by NAPI */
    work_done = min(budget, 10);

    /* Indicate if more work is remaining */
    if (work_done < budget) {
        napi_complete_done(napi, work_done);
#if 0
        /* Enable the interrupt again */
        enable_irq(napi_netdev_b->irq);
#endif
    }

    return work_done;
}

static int napi_open_a(struct net_device *dev)
{
    printk(KERN_INFO "NAPI open function called\n");

    napi_enable(&napi_a);
    /* Enable interrupts */
    enable_irq(dev->irq);

    mod_timer(&napi_a_trigger_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_A));

    return 0;
}

static int napi_stop_a(struct net_device *dev)
{
    printk(KERN_INFO "NAPI stop function called\n");

    del_timer(&napi_a_trigger_timer);

    napi_disable(&napi_a);
    disable_irq(dev->irq);

    return 0;
}

static int napi_open_b(struct net_device *dev)
{
    printk(KERN_INFO "NAPI open function called\n");

    napi_enable(&napi_b);
    napi_enable(&napi_b_1);
    napi_enable(&napi_b_2);
    napi_enable(&napi_b_3);
    napi_enable(&napi_b_4);
    napi_enable(&napi_b_5);
    /* Enable interrupts */
    enable_irq(dev->irq);

    mod_timer(&napi_b_trigger_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_B));

    return 0;
}

static int napi_stop_b(struct net_device *dev)
{
    printk(KERN_INFO "NAPI stop function called\n");

    del_timer(&napi_b_trigger_timer);

    napi_disable(&napi_b);
    napi_disable(&napi_b_1);
    napi_disable(&napi_b_2);
    napi_disable(&napi_b_3);
    napi_disable(&napi_b_4);
    napi_disable(&napi_b_5);
    /* Disable interrupts */
    disable_irq(dev->irq);

    return 0;
}

static int napi_open_c(struct net_device *dev)
{
    printk(KERN_INFO "[%s] NAPI open function called\n", __func__);

    napi_enable(&napi_c);
    enable_irq(dev->irq);

    mod_timer(&napi_c_trigger_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_C));
#if 0
    mod_timer(&napi_c_1_trigger_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_C_1));
#endif

    return 0;
}

static int napi_stop_c(struct net_device *dev)
{
    printk(KERN_INFO "NAPI stop function called\n");

    del_timer(&napi_c_trigger_timer);
#if 0
    del_timer(&napi_c_1_trigger_timer);
#endif

    napi_disable(&napi_c);
    disable_irq(dev->irq);

    return 0;
}

static
int napi_dev_xmit(struct sk_buff *skb, struct net_device *dev)
{
    return 0;
}

/* Network device operations */
static const struct net_device_ops napi_netdev_ops_a = {
    .ndo_open = napi_open_a,
    .ndo_stop = napi_stop_a,
    .ndo_start_xmit = napi_dev_xmit,
};


/* Network device operations */
static const struct net_device_ops napi_netdev_ops_b = {
    .ndo_open = napi_open_b,
    .ndo_stop = napi_stop_b,
    .ndo_start_xmit = napi_dev_xmit,
};

/* Network device operations */
static const struct net_device_ops napi_netdev_ops_c = {
    .ndo_open = napi_open_c,
    .ndo_stop = napi_stop_c,
    .ndo_start_xmit = napi_dev_xmit,
};

static void napi_setup_a(struct net_device *dev)
{
    ether_setup(dev);

    dev->netdev_ops = &napi_netdev_ops_a;
    dev->irq = 1;  // Assign a dummy IRQ number for the example
}

static void napi_setup_b(struct net_device *dev)
{
    ether_setup(dev);

    dev->netdev_ops = &napi_netdev_ops_b;
    dev->irq = 1;  // Assign a dummy IRQ number for the example
}

static void napi_setup_c(struct net_device *dev)
{
    ether_setup(dev);

    dev->netdev_ops = &napi_netdev_ops_c;
    dev->irq = 1;  // Assign a dummy IRQ number for the example
}

/* Module init function */
static int __init napi_example_init(void)
{
    int ret;

    printk(KERN_INFO "NAPI example module loaded\n");

    napi_netdev_a = alloc_netdev(0, "napi%d", NET_NAME_UNKNOWN, napi_setup_a);
    if (!napi_netdev_a) {
        printk(KERN_ERR "Failed to allocate net_device\n");
        return -ENOMEM;
    }

    napi_netdev_b = alloc_netdev(0, "napi%d", NET_NAME_UNKNOWN, napi_setup_b);
    if (!napi_netdev_b) {
        printk(KERN_ERR "Failed to allocate net_device\n");
        free_netdev(napi_netdev_a);
        return -ENOMEM;
    }

    napi_netdev_c = alloc_netdev(0, "napi%d", NET_NAME_UNKNOWN, napi_setup_c);
    if (!napi_netdev_b) {
        printk(KERN_ERR "Failed to allocate net_device\n");
        free_netdev(napi_netdev_a);
        free_netdev(napi_netdev_b);
        return -ENOMEM;
    }

    napi_netdev_c_1 = alloc_netdev(0, "napi%d", NET_NAME_UNKNOWN, napi_setup_c);
    if (!napi_netdev_b) {
        printk(KERN_ERR "Failed to allocate net_device\n");
        free_netdev(napi_netdev_a);
        free_netdev(napi_netdev_b);
        free_netdev(napi_netdev_c);
        return -ENOMEM;
    }

    ret = register_netdev(napi_netdev_a);
    if (ret) {
        printk(KERN_ERR "Failed to register net_device\n");
        free_netdev(napi_netdev_a);
        free_netdev(napi_netdev_b);
        free_netdev(napi_netdev_c);
        free_netdev(napi_netdev_c_1);
        return ret;
    }

    ret = register_netdev(napi_netdev_b);
    if (ret) {
        printk(KERN_ERR "Failed to register net_device\n");
	unregister_netdev(napi_netdev_a);
        free_netdev(napi_netdev_a);
        free_netdev(napi_netdev_b);
	free_netdev(napi_netdev_c);
        free_netdev(napi_netdev_c_1);
        return ret;
    }

    ret = register_netdev(napi_netdev_c);
    if (ret) {
        printk(KERN_ERR "Failed to register net_device\n");
	unregister_netdev(napi_netdev_a);
	unregister_netdev(napi_netdev_b);
        free_netdev(napi_netdev_a);
        free_netdev(napi_netdev_b);
	free_netdev(napi_netdev_c);
        free_netdev(napi_netdev_c_1);
        return ret;
    }

    ret = register_netdev(napi_netdev_c_1);
    if (ret) {
        printk(KERN_ERR "Failed to register net_device\n");
	unregister_netdev(napi_netdev_a);
	unregister_netdev(napi_netdev_b);
	unregister_netdev(napi_netdev_c);
        free_netdev(napi_netdev_a);
        free_netdev(napi_netdev_b);
	free_netdev(napi_netdev_c);
        free_netdev(napi_netdev_c_1);
        return ret;
    }

    netif_napi_add(napi_netdev_a, &napi_a, napi_poll_a);

    dev_set_threaded(napi_netdev_b, 1);
    netif_napi_add(napi_netdev_b, &napi_b, napi_poll_b);
    netif_napi_add(napi_netdev_b, &napi_b_1, napi_poll_b_1);
    netif_napi_add(napi_netdev_b, &napi_b_2, napi_poll_b_2);
    netif_napi_add(napi_netdev_b, &napi_b_3, napi_poll_b_3);
    netif_napi_add(napi_netdev_b, &napi_b_4, napi_poll_b_4);
    netif_napi_add(napi_netdev_b, &napi_b_5, napi_poll_b_5);

    dev_set_threaded(napi_netdev_c, 1);
    netif_napi_add(napi_netdev_c, &napi_c, napi_poll_c);
#if 0
    netif_napi_add(napi_netdev_c_1, &napi_c, napi_poll_c_1);
#endif

    timer_setup(&napi_a_trigger_timer, napi_a_trigger_timer_cb, 0);
    timer_setup(&napi_b_trigger_timer, napi_b_trigger_timer_cb, 0);
    timer_setup(&napi_c_trigger_timer, napi_c_trigger_timer_cb, 0);
#if 0
    timer_setup(&napi_c_1_trigger_timer, napi_c_1_trigger_timer_cb, 0);
#endif

   return 0;
}

/* Module exit function */
static void __exit napi_example_exit(void)
{
    printk(KERN_INFO "NAPI example module unloaded\n");

    unregister_netdev(napi_netdev_a);
    unregister_netdev(napi_netdev_b);
    unregister_netdev(napi_netdev_c);
    unregister_netdev(napi_netdev_c_1);
#if 0
    netif_napi_del(&napi_a);
    netif_napi_del(&napi_b);
    netif_napi_del(&napi_b_1);
    netif_napi_del(&napi_b_2);
    netif_napi_del(&napi_b_3);
    netif_napi_del(&napi_b_4);
    netif_napi_del(&napi_b_5);
#endif
    free_netdev(napi_netdev_a);
    free_netdev(napi_netdev_b);
    free_netdev(napi_netdev_c);
    free_netdev(napi_netdev_c_1);
}

module_init(napi_example_init);
module_exit(napi_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("A simple module demonstrating NAPI usage");

