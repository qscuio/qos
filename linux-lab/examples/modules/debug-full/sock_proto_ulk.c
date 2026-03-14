#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/uio.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>
#include <net/sock.h>

struct ulk_sock
{
    /* Must be first member */
    struct sock sk;
    struct packet_type pack ____cacheline_aligned_in_smp;
    struct timer_list timer;
    pid_t pid;
};
#define ulk_sk(ptr) container_of_const(ptr, struct ulk_sock, sk)

static const struct proto_ops ulk_ops;
 /* struct socket - general BSD socket */
#define TIMER_INTERVAL_SEC 1

static void hex_dump(char *func, const unsigned char *buffer, size_t length); 

static int dev_pkt_rcv(struct sk_buff *skb, struct net_device *dev,
                         struct packet_type *pt, struct net_device *orig_dev);

#define dev_define_pack_by_name(name, ptype, handler)           \
__maybe_unused static struct packet_type __##name##_packet_type;               \
__maybe_unused static void dev_add_pack_##name(const char *name)               \
{                                                               \
    struct net_device *dev;                                     \
                                                                \
    dev = dev_get_by_name(&init_net, name);                     \
                                                                \
    __##name##_packet_type.type = htons(ptype);                 \
    __##name##_packet_type.dev = dev;                           \
    __##name##_packet_type.func = handler;                      \
    dev_add_pack(&__##name##_packet_type);                      \
    if (dev)                                                    \
    dev_put(dev);                                               \
                                                                \
    return;                                                     \
}

#define dev_add_pack_by_name(name)                              \
    dev_add_pack_##name(#name)

#define dev_del_pack_by_name(name)                              \
    dev_remove_pack(&__##name##_packet_type)

dev_define_pack_by_name(all, ETH_P_ALL, dev_pkt_rcv);
dev_define_pack_by_name(eth01, ETH_P_ALL, dev_pkt_rcv);

static struct proto ulk_proto = {
    .name       = "AF_ULK",
    .owner      = THIS_MODULE,
    .obj_size   = sizeof(struct ulk_sock),
};

__maybe_unused static void hex_dump(char *func, const unsigned char *buffer, size_t length) {
    printk("--------%s-------\n", func);
    const size_t bytes_per_line = 16;
    for (size_t i = 0; i < length; i += bytes_per_line) {
        printk("%08zx  ", i);

        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < length) {
                printk("%02x ", buffer[i + j]);
            } else {
                printk("   ");
            }
        }

        printk(" |");

        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < length) {
                unsigned char c = buffer[i + j];
                printk("%c", isprint(c) ? c : '.');
            } else {
                printk(" ");
            }
        }

        printk("|\n");
    }
}

static int dev_pkt_rcv(struct sk_buff *skb, struct net_device *dev,
                         struct packet_type *pt, struct net_device *orig_dev)
{
    /*
     *	struct sock - network layer representation of sockets
     */

    struct sock *sk = NULL;
    struct ulk_sock *usk = pt->af_packet_priv;
    struct sk_buff *skb2 = NULL;

    printk(KERN_INFO "[%d][%s]: Received packet on dev %s\n", usk->pid, __func__, dev->name);


    sk = &usk->sk;

    skb2 = skb_clone(skb, GFP_ATOMIC);
    if (!skb2)
    {
	printk("Failed to clone skb\n");
	kfree(skb2);
	return NET_RX_DROP;
    }

    skb_queue_tail(&sk->sk_receive_queue, skb2);
    sk->sk_data_ready(sk);

    /* Free the packet */
    kfree_skb(skb);

    return NET_RX_SUCCESS;
}

static void ulk_packet_generator(struct timer_list *t)
{
    struct sk_buff *skb;
    struct msghdr msg;
    struct ulk_sock *usk = container_of(t, struct ulk_sock, timer);
    struct sock *sk = NULL;
    char *rx = "Hello user, I'm kernel";
    int len = strlen(rx) + 1;

    printk(KERN_INFO "[%d][%s] called..\n", usk->pid, __func__);

    sk = &usk->sk;

    skb = __alloc_skb(len, GFP_ATOMIC, SKB_ALLOC_RX, NUMA_NO_NODE);
    if (!skb)
        return;

#if 1
    skb_put_data(skb, rx, len);
#else
    skb->len = len;
    memcpy(skb->data, rx, len);
#endif

    memset(&msg, 0, sizeof(msg));
    
    skb_queue_tail(&sk->sk_receive_queue, skb);

    sk->sk_data_ready(sk);

    mod_timer(&usk->timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_SEC * 1000));

    return;
}

static int ulk_create(struct net *net, struct socket *sock, int protocol, int kern)
{
    struct sock *sk;
    struct ulk_sock *usk = NULL; 

    printk(KERN_INFO "ulk[%d][%s]: called.\n", current->pid, __func__);
    dump_stack();
    sk = sk_alloc(net, AF_ULK, GFP_KERNEL, &ulk_proto, kern);
    if (!sk)
        return -ENOMEM;

    sock->ops = &ulk_ops;

    sock_init_data(sock, sk);
    
    usk = ulk_sk(sk);
    usk->pack.func = dev_pkt_rcv;
    usk->pack.type = htons(ETH_P_ALL);
    usk->pack.af_packet_priv = sk;
    dev_add_pack(&usk->pack);

    timer_setup(&usk->timer, ulk_packet_generator, 0);
    mod_timer(&usk->timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_SEC * 1000));

    usk->pid = current->pid;

    return 0;
}

static int ulk_release(struct socket *sock)
{
    struct sock *sk = sock->sk;
    struct ulk_sock *usk = ulk_sk(sk); 

    dump_stack();
    printk(KERN_INFO "ulk[%d][%s]: called.\n", usk->pid, __func__);

    if (sk) {
	del_timer(&usk->timer);
	dev_remove_pack(&usk->pack);
	usk->pid = 0;

	sock_orphan(sk);
	sock_put(sk);
	sock->sk = NULL;
    }

    return 0;
}

static int ulk_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
{
    struct ulk_sock *usk = ulk_sk(sock->sk); 

    printk(KERN_INFO "ulk[%d][%s]: called.\n", usk->pid, __func__);
    dump_stack();
    return 0;
}

static int ulk_sendmsg(struct socket *sock, struct msghdr *msg, size_t len)
{
    char *buf = NULL; 
    struct ulk_sock *usk = ulk_sk(sock->sk); 
    
    printk(KERN_INFO "ulk[%d][%s]: called.\n", usk->pid, __func__);
    buf = kmalloc(len, GFP_ATOMIC);
    if (buf)
    {
	memcpy_from_msg(buf, msg, len);
	printk(KERN_INFO "Message %s", buf);
    }
    dump_stack();
    return len;
}

static int ulk_recvmsg(struct socket *sock, struct msghdr *msg, size_t len, int flags)
{
    int err = 0;
    struct sk_buff *skb;
    struct sock *sk = sock->sk;
    struct ulk_sock *usk = ulk_sk(sock->sk); 
    char *rx = "Hello user, I'm kernel";

    printk(KERN_INFO "ulk[%d][%s]: called.\n", usk->pid, __func__);

#if 1
    skb = skb_recv_datagram(sk, flags, &err);
    if (!skb)
	return err;
#else
    skb = __alloc_skb(strlen(rx) + 1, GFP_ATOMIC, SKB_ALLOC_RX, NUMA_NO_NODE);
    if (!skb)
        return -ENOMEM;

    skb->len = strlen(rx) + 1;
    memcpy(skb->data, rx, strlen(rx) + 1);
#endif
    err = skb_copy_datagram_msg(skb, 0, msg, len);

    skb_free_datagram(sk, skb);
    dump_stack();

    return strlen(rx)+1;
}

static const struct proto_ops ulk_ops = {
    .family         = AF_ULK,
    .owner          = THIS_MODULE,
    .release        = ulk_release,
    .bind           = ulk_bind,
    .connect        = sock_no_connect,
    .socketpair     = sock_no_socketpair,
    .accept         = sock_no_accept,
    .getname        = sock_no_getname,
    .poll           = datagram_poll,
    .ioctl          = sock_no_ioctl,
    .listen         = sock_no_listen,
    .shutdown       = sock_no_shutdown,
    .setsockopt     = sock_common_setsockopt,
    .getsockopt     = sock_common_getsockopt,
    .sendmsg        = ulk_sendmsg,
    .recvmsg        = ulk_recvmsg,
    .mmap           = sock_no_mmap,
};

static struct net_proto_family ulk_family_ops = {
    .family = PF_ULK,
    .create = ulk_create,
    .owner  = THIS_MODULE,
};

static int __init ulk_init(void)
{
    int ret;

    ret = proto_register(&ulk_proto, 0);
    if (ret)
        return ret;

    ret = sock_register(&ulk_family_ops);
    if (ret) {
        proto_unregister(&ulk_proto);
        return ret;
    }

#if 0
    dev_add_pack_by_name(eth01);
    dev_add_pack_by_name(all);
#endif

    printk(KERN_INFO "ulk: protocol family registered.\n");
    return 0;
}

static void __exit ulk_exit(void)
{
#if 0
    dev_del_pack_by_name(all);
    dev_del_pack_by_name(eth01);
#endif
    sock_unregister(PF_ULK);
    proto_unregister(&ulk_proto);
    printk(KERN_INFO "ulk: protocol family unregistered.\n");
}

module_init(ulk_init);
module_exit(ulk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("A simple example of a new socket protocol family with custom handlers.");

