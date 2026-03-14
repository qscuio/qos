#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/rtnetlink.h>

static struct net_device *dev1, *dev2;
static struct packet_type all_packet_type;

/* RX Handler for the first device */
static rx_handler_result_t dev1_rx_handler(struct sk_buff **pskb)
{
    struct sk_buff *skb = skb_clone(*pskb, GFP_ATOMIC);

    printk(KERN_INFO "dev1_rx_handler: Received packet on dev1\n");

    /* Process packet here */
    /* Drop the packet */
    kfree_skb(skb);
    return RX_HANDLER_PASS;
}

/* RX Handler for the second device */
static rx_handler_result_t dev2_rx_handler(struct sk_buff **pskb)
{
    struct sk_buff *skb = skb_clone(*pskb, GFP_ATOMIC);

    printk(KERN_INFO "dev2_rx_handler: Received packet on dev2\n");

    /* Process packet here */
    /* Drop the packet */
    kfree_skb(skb);
    return RX_HANDLER_PASS;
}

/* Packet handler function */
static int dev_packet_rcv(struct sk_buff *skb, struct net_device *dev,
                         struct packet_type *pt, struct net_device *orig_dev)
{
    struct sk_buff *skb2 = skb_clone(skb, GFP_ATOMIC);
    printk(KERN_INFO "[%s]: Received packet on dev %s\n", __func__, dev->name);

    /* Process packet here */

    /* Free the packet */
    kfree_skb(skb2);
    return NET_RX_SUCCESS;
}

/* Net device open function */
static int my_netdev_open(struct net_device *dev)
{
    printk(KERN_INFO "%s: device opened\n", dev->name);
    netif_start_queue(dev);
    return 0;
}

/* Net device stop function */
static int my_netdev_stop(struct net_device *dev)
{
    printk(KERN_INFO "%s: device closed\n", dev->name);
    netif_stop_queue(dev);
    return 0;
}

/* Net device start xmit function */
static netdev_tx_t my_netdev_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    printk(KERN_INFO "%s: start xmit\n", dev->name);
    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

/* Define net_device_ops */
static const struct net_device_ops my_netdev_ops = {
    .ndo_open = my_netdev_open,
    .ndo_stop = my_netdev_stop,
    .ndo_start_xmit = my_netdev_start_xmit,
};

/* Initialize net_device structures */
static void setup_net_device(struct net_device *dev, char *name, rx_handler_func_t handler)
{
    ether_setup(dev);
    dev->netdev_ops = &my_netdev_ops;
    dev->watchdog_timeo = 5 * HZ;
    strcpy(dev->name, name);

    /* Register the RX handler */
    rtnl_lock();
    netdev_rx_handler_register(dev, handler, NULL);
    rtnl_unlock();
}

/* RX Handler for eth02 */
__maybe_unused static rx_handler_result_t eth02_rx_handler(struct sk_buff **pskb)
{
    //struct sk_buff *skb = skb_clone(*pskb, GFP_ATOMIC);
    struct sk_buff *skb = *pskb;

    printk(KERN_INFO "eth02_rx_handler: Received packet on eth02\n");

    /* Process packet here */
    /* Drop the packet */
    kfree_skb(skb);
    return RX_HANDLER_CONSUMED;
}

/* Packet handler function */
static int dev_pkt_rcv(struct sk_buff *skb, struct net_device *dev,
                         struct packet_type *pt, struct net_device *orig_dev)
{
    struct sk_buff *skb2 = skb_clone(skb, GFP_ATOMIC);
    printk(KERN_INFO "[%s]: Received packet on dev %s\n", __func__, dev->name);

    /* Process packet here */

    /* Free the packet */
    kfree_skb(skb2);
    return NET_RX_SUCCESS;
}

#define dev_define_pack_by_name(name, ptype, handler)           \
static struct packet_type __##name##_packet_type;               \
static void dev_add_pack_##name(const char *name)               \
{                                                               \
    struct net_device *dev;                                     \
                                                                \
    dev = dev_get_by_name(&init_net, name);                     \
                                                                \
    __##name##_packet_type.type = htons(ptype);                 \
    __##name##_packet_type.dev = dev;                           \
    __##name##_packet_type.func = handler;                      \
    dev_add_pack(&__##name##_packet_type);                      \
    dev_put(dev);                                               \
                                                                \
    return;                                                     \
}

#define dev_add_pack_by_name(name)                              \
    dev_add_pack_##name(#name)

#define dev_del_pack_by_name(name)                              \
    dev_remove_pack(&__##name##_packet_type)

dev_define_pack_by_name(eth01, ETH_P_ALL, dev_pkt_rcv);
dev_define_pack_by_name(eth02, ETH_P_ALL, dev_pkt_rcv);

__maybe_unused static int dev_add_handler_by_name(char *name, rx_handler_func_t handler)
{
    int ret;
    struct net_device *dev;

    dev = dev_get_by_name(&init_net, name);
    if (!dev) {
	printk(KERN_ERR "Failed to find eth02\n");
	return -ENODEV;
    }

    /* Register the RX handler */
    rtnl_lock();
    ret = netdev_rx_handler_register(dev, handler, NULL);
    rtnl_unlock();
    if (ret) {
        printk(KERN_ERR "Failed to register rx handler for %s\n", name);
        dev_put(dev);
        return ret;
    }
    dev_put(dev);

    return ret;
}

__maybe_unused static int dev_del_handler_by_name(char *name)
{
    int ret;
    struct net_device *dev;

    dev = dev_get_by_name(&init_net, name);
    if (!dev) {
	printk(KERN_ERR "Failed to find eth02\n");
	return -ENODEV;
    }

    /* Register the RX handler */
    rtnl_lock();
    netdev_rx_handler_unregister(dev);
    rtnl_unlock();
    if (ret) {
        printk(KERN_ERR "Failed to unregister rx handler for %s\n", name);
        dev_put(dev);
        return ret;
    }
    dev_put(dev);

    return ret;
}

static int __init my_module_init(void)
{
    int ret = 0;

    dev_add_pack_by_name(eth01);
    dev_add_pack_by_name(eth02);

    /* Allocate and initialize dev1 */
    dev1 = alloc_netdev(0, "dev1", NET_NAME_UNKNOWN, ether_setup);
    if (!dev1) {
        printk(KERN_ERR "Failed to allocate dev1\n");
        return -ENOMEM;
    }
    setup_net_device(dev1, "dev1", dev1_rx_handler);

    /* Allocate and initialize dev2 */
    dev2 = alloc_netdev(0, "dev2", NET_NAME_UNKNOWN, ether_setup);
    if (!dev2) {
        printk(KERN_ERR "Failed to allocate dev2\n");
        free_netdev(dev1);
        return -ENOMEM;
    }
    setup_net_device(dev2, "dev2", dev2_rx_handler);

    /* Register devices */
    ret = register_netdev(dev1);
    if (ret) {
        printk(KERN_ERR "Failed to register dev1\n");
        free_netdev(dev1);
        free_netdev(dev2);
        return ret;
    }

    ret = register_netdev(dev2);
    if (ret) {
        printk(KERN_ERR "Failed to register dev2\n");
        unregister_netdev(dev1);
        free_netdev(dev1);
        free_netdev(dev2);
        return ret;
    }

    /* Set up packet type for custom packet handling */
    all_packet_type.type = htons(ETH_P_ALL);
    all_packet_type.dev = NULL; /* NULL means all devices */
    all_packet_type.func = dev_packet_rcv;
    dev_add_pack(&all_packet_type);

    printk(KERN_INFO "Module loaded\n");
    return 0;
}

static void __exit my_module_exit(void)
{
    dev_del_handler_by_name("eth01");
    dev_del_handler_by_name("eth02");

    dev_del_pack_by_name(eth01);
    dev_del_pack_by_name(eth02);

    dev_remove_pack(&all_packet_type);
    /* Unregister devices */
    unregister_netdev(dev1);
    unregister_netdev(dev2);

    /* Free devices */
    free_netdev(dev1);
    free_netdev(dev2);

    printk(KERN_INFO "Module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Example module demonstrating dev_add_pack, dev_remove_pack, and rx_handler");
MODULE_AUTHOR("qwert");

