#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <net/netlink.h>
#include <net/net_namespace.h>
#include <linux/skbuff.h>

#define NETLINK_USER 31  // Custom Netlink family ID
#define MSG_LEN 128      // Maximum message length

struct sock *netlink_socket;  // Netlink socket pointer
static void netlink_send_message(int pid, char *message);
static void netlink_receive_message(struct sk_buff *skb); 

static void netlink_receive_message(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    char *message;
    int pid;

    nlh = (struct nlmsghdr *)skb->data;
    message = NLMSG_DATA(nlh);
    pid = nlh->nlmsg_pid;  // PID of sending process

    printk(KERN_INFO "Netlink message received from PID %d: %s\n", pid, message);

    // Echo the message back to the sender
    netlink_send_message(pid, message);
}

static void netlink_send_message(int pid, char *message) {
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int message_size = strlen(message) + 1;

    skb_out = nlmsg_new(message_size, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, message_size, 0);
    strncpy(NLMSG_DATA(nlh), message, message_size);

    //NETLINK_CB(skb_out).pid = 0;  // Kernel PID is 0 (unimportant in this context)
    NETLINK_CB(skb_out).dst_group = 0;  // Unicast message

    nlmsg_unicast(netlink_socket, skb_out, pid);
}

static int __init netlink_example_init(void) {
    struct netlink_kernel_cfg cfg = {
        .input = netlink_receive_message,
    };

    netlink_socket = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!netlink_socket) {
        printk(KERN_ERR "Failed to create Netlink socket\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "Netlink module initialized\n");
    return 0;
}

static void __exit netlink_example_exit(void) {
    if (netlink_socket) {
        netlink_kernel_release(netlink_socket);
    }
    printk(KERN_INFO "Netlink module exited\n");
}

module_init(netlink_example_init);
module_exit(netlink_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Netlink example module");

