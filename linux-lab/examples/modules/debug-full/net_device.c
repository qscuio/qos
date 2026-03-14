#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux Netdevice Example");
MODULE_VERSION("0.1");

#define VIRTUAL_INTERFACE_NAME "my_netdev"

static struct net_device *my_netdev;

static int my_netdev_open(struct net_device *dev) {
    // This function is called when the network interface is brought up (ifconfig up)
    printk(KERN_INFO "my_netdev: Interface is up\n");
    netif_start_queue(dev);
    return 0;
}

static int my_netdev_stop(struct net_device *dev) {
    // This function is called when the network interface is brought down (ifconfig down)
    printk(KERN_INFO "my_netdev: Interface is down\n");
    netif_stop_queue(dev);
    return 0;
}

static netdev_tx_t my_netdev_xmit(struct sk_buff *skb, struct net_device *dev) {
    // This function is called when data is sent through the network interface
    printk(KERN_INFO "my_netdev: Transmitting data\n");
    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

static struct net_device_ops my_netdev_ops = {
    .ndo_open = my_netdev_open,
    .ndo_stop = my_netdev_stop,
    .ndo_start_xmit = my_netdev_xmit,
};

static void my_netdev_setup(struct net_device *dev) {
    // Initialize the net_device structure
    ether_setup(dev);

    // Assign the device operations
    dev->netdev_ops = &my_netdev_ops;
}

static int __init example_module_init(void) {
    int ret = 0;

    pr_info("Example module initializing\n");

    // Create a new network device
    my_netdev = alloc_netdev(0, VIRTUAL_INTERFACE_NAME, NET_NAME_UNKNOWN, my_netdev_setup);
    if (!my_netdev) {
        pr_err("Failed to allocate netdev\n");
        return -ENOMEM;
    }

    // Register the network device with the kernel
    ret = register_netdev(my_netdev);
    if (ret) {
        pr_err("Failed to register netdev\n");
        free_netdev(my_netdev);
        return ret;
    }

    return 0;
}

static void __exit example_module_exit(void) {
    pr_info("Example module exiting\n");

    // Unregister and free the network device
    unregister_netdev(my_netdev);
    free_netdev(my_netdev);
}

module_init(example_module_init);
module_exit(example_module_exit);

