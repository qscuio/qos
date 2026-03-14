#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#define NETDEV_NAME "my_netdev"
#define ETH_ALEN 6

static struct net_device *netdev;

static int netdev_open(struct net_device *dev)
{
    printk(KERN_INFO "%s: device opened\n", dev->name);
    netif_start_queue(dev);  // Start network queue
    return 0;
}

static int netdev_stop(struct net_device *dev)
{
    printk(KERN_INFO "%s: device closed\n", dev->name);
    netif_stop_queue(dev);  // Stop network queue
    return 0;
}

static netdev_tx_t netdev_tx(struct sk_buff *skb, struct net_device *dev)
{
    printk(KERN_INFO "%s: transmitting packet\n", dev->name);
    dev_kfree_skb(skb);  // Free the sk_buff buffer
    return NETDEV_TX_OK; // Transmission successful
}

static struct net_device_ops netdev_ops = {
    .ndo_open = netdev_open,
    .ndo_stop = netdev_stop,
    .ndo_start_xmit = netdev_tx,
};

static void netdev_setup(struct net_device *dev)
{
    ether_setup(dev); // Initialize net_device with Ethernet defaults
    dev->netdev_ops = &netdev_ops;
    memcpy(dev->dev_addr, "\xAA\xBB\xCC\xDD\xEE\xFF", ETH_ALEN); // Set MAC address
}

static int __init netdev_init(void)
{
    netdev = alloc_netdev(0, NETDEV_NAME, netdev_setup);
    if (!netdev) {
        printk(KERN_ERR "Failed to allocate netdev\n");
        return -ENOMEM;
    }
    register_netdev(netdev);
    printk(KERN_INFO "Registered netdev: %s\n", netdev->name);
    return 0;
}

static void __exit netdev_exit(void)
{
    unregister_netdev(netdev);
    free_netdev(netdev);
    printk(KERN_INFO "Unregistered netdev: %s\n", netdev->name);
}

module_init(netdev_init);
module_exit(netdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Network Device Driver");

