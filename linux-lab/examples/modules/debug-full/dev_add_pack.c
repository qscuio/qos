#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("Example module using dev_add_pack and dev->rx_handler");
MODULE_VERSION("1.0");

static struct packet_type my_packet_type;
static struct net_device *handler_dev = NULL;
static rx_handler_result_t my_rx_handler(struct sk_buff **pskb);

static int my_packet_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
{
    pr_info("my_packet_rcv: Packet received on %s\n", dev->name);
    kfree_skb(skb); // Free the socket buffer
    return NET_RX_DROP;
}

static rx_handler_result_t my_rx_handler(struct sk_buff **pskb)
{
    struct sk_buff *skb = *pskb;
    struct ethhdr *eth = eth_hdr(skb);

    pr_info("my_rx_handler: Packet received on %s\n", skb->dev->name);

    if (eth->h_proto == htons(ETH_P_IP)) {
        pr_info("my_rx_handler: Handling IP packet\n");
    }

    return RX_HANDLER_PASS; // Pass the packet to the next handler
}

static int __init dev_module_init(void)
{
    pr_info("Initializing module\n");

    // Register packet handler for all IP packets (protocol ETH_P_IP)
    my_packet_type.type = htons(ETH_P_IP);
    my_packet_type.dev = NULL; // NULL means all devices
    my_packet_type.func = my_packet_rcv;
    dev_add_pack(&my_packet_type);

    // Set up rx_handler for a specific network device (e.g., eth0)
    handler_dev = dev_get_by_name(&init_net, "eth0");
    if (handler_dev) {
        rtnl_lock();
        netdev_rx_handler_register(handler_dev, my_rx_handler, NULL);
        rtnl_unlock();
    } else {
        pr_err("Failed to get network device\n");
    }

    return 0;
}

static void __exit dev_module_exit(void)
{
    pr_info("Exiting module\n");

    // Unregister packet handler
    dev_remove_pack(&my_packet_type);

    // Unregister rx_handler
    if (handler_dev) {
        rtnl_lock();
        netdev_rx_handler_unregister(handler_dev);
        rtnl_unlock();
        dev_put(handler_dev);
    }
}

module_init(dev_module_init);
module_exit(dev_module_exit);

