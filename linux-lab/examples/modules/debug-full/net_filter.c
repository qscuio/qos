#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("An IPv4 and IPv6 packet sniffer");

static struct nf_hook_ops nfho_rx_ipv4, nfho_tx_ipv4, nfho_rx_ipv6, nfho_tx_ipv6;

static unsigned int hook_func_rx_ipv4(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);

    if (!ip_header)
        return NF_ACCEPT;

    printk(KERN_INFO "RX IPv4 Packet - Source: %pI4, Destination: %pI4, Protocol: %u\n",
           &ip_header->saddr, &ip_header->daddr, ip_header->protocol);

    if (ip_header->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp_header = (struct tcphdr *)((__u32 *)ip_header + ip_header->ihl);
        printk(KERN_INFO "TCP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(tcp_header->source), ntohs(tcp_header->dest));
    } else if (ip_header->protocol == IPPROTO_UDP) {
        struct udphdr *udp_header = (struct udphdr *)((__u32 *)ip_header + ip_header->ihl);
        printk(KERN_INFO "UDP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(udp_header->source), ntohs(udp_header->dest));
    }

    return NF_ACCEPT;
}

static unsigned int hook_func_tx_ipv4(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);

    if (!ip_header)
        return NF_ACCEPT;

    printk(KERN_INFO "TX IPv4 Packet - Source: %pI4, Destination: %pI4, Protocol: %u\n",
           &ip_header->saddr, &ip_header->daddr, ip_header->protocol);

    if (ip_header->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp_header = (struct tcphdr *)((__u32 *)ip_header + ip_header->ihl);
        printk(KERN_INFO "TCP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(tcp_header->source), ntohs(tcp_header->dest));
    } else if (ip_header->protocol == IPPROTO_UDP) {
        struct udphdr *udp_header = (struct udphdr *)((__u32 *)ip_header + ip_header->ihl);
        printk(KERN_INFO "UDP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(udp_header->source), ntohs(udp_header->dest));
    }

    return NF_ACCEPT;
}

static unsigned int hook_func_rx_ipv6(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct ipv6hdr *ip6_header = (struct ipv6hdr *)skb_network_header(skb);

    if (!ip6_header)
        return NF_ACCEPT;

    printk(KERN_INFO "RX IPv6 Packet - Source: %pI6, Destination: %pI6, Next Header: %u\n",
           &ip6_header->saddr, &ip6_header->daddr, ip6_header->nexthdr);

    if (ip6_header->nexthdr == IPPROTO_TCP) {
        struct tcphdr *tcp_header = (struct tcphdr *)(skb_transport_header(skb));
        printk(KERN_INFO "TCP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(tcp_header->source), ntohs(tcp_header->dest));
    } else if (ip6_header->nexthdr == IPPROTO_UDP) {
        struct udphdr *udp_header = (struct udphdr *)(skb_transport_header(skb));
        printk(KERN_INFO "UDP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(udp_header->source), ntohs(udp_header->dest));
    }

    return NF_ACCEPT;
}

static unsigned int hook_func_tx_ipv6(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct ipv6hdr *ip6_header = (struct ipv6hdr *)skb_network_header(skb);

    if (!ip6_header)
        return NF_ACCEPT;

    printk(KERN_INFO "TX IPv6 Packet - Source: %pI6, Destination: %pI6, Next Header: %u\n",
           &ip6_header->saddr, &ip6_header->daddr, ip6_header->nexthdr);

    if (ip6_header->nexthdr == IPPROTO_TCP) {
        struct tcphdr *tcp_header = (struct tcphdr *)(skb_transport_header(skb));
        printk(KERN_INFO "TCP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(tcp_header->source), ntohs(tcp_header->dest));
    } else if (ip6_header->nexthdr == IPPROTO_UDP) {
        struct udphdr *udp_header = (struct udphdr *)(skb_transport_header(skb));
        printk(KERN_INFO "UDP Packet - Source Port: %u, Destination Port: %u\n",
               ntohs(udp_header->source), ntohs(udp_header->dest));
    }

    return NF_ACCEPT;
}

static int __init packet_sniffer_init(void) {
    nfho_rx_ipv4.hook = hook_func_rx_ipv4;
    nfho_rx_ipv4.hooknum = NF_INET_PRE_ROUTING;
    nfho_rx_ipv4.pf = PF_INET;
    nfho_rx_ipv4.priority = NF_IP_PRI_FIRST;

    nfho_tx_ipv4.hook = hook_func_tx_ipv4;
    nfho_tx_ipv4.hooknum = NF_INET_POST_ROUTING;
    nfho_tx_ipv4.pf = PF_INET;
    nfho_tx_ipv4.priority = NF_IP_PRI_FIRST;

    nfho_rx_ipv6.hook = hook_func_rx_ipv6;
    nfho_rx_ipv6.hooknum = NF_INET_PRE_ROUTING;
    nfho_rx_ipv6.pf = PF_INET6;
    nfho_rx_ipv6.priority = NF_IP_PRI_FIRST;

    nfho_tx_ipv6.hook = hook_func_tx_ipv6;
    nfho_tx_ipv6.hooknum = NF_INET_POST_ROUTING;
    nfho_tx_ipv6.pf = PF_INET6;
    nfho_tx_ipv6.priority = NF_IP_PRI_FIRST;

    nf_register_net_hook(&init_net, &nfho_rx_ipv4);
    nf_register_net_hook(&init_net, &nfho_tx_ipv4);
    nf_register_net_hook(&init_net, &nfho_rx_ipv6);
    nf_register_net_hook(&init_net, &nfho_tx_ipv6);

    printk(KERN_INFO "Packet sniffer module loaded\n");

    return 0;
}

static void __exit packet_sniffer_exit(void) {
    nf_unregister_net_hook(&init_net, &nfho_rx_ipv4);
    nf_unregister_net_hook(&init_net, &nfho_tx_ipv4);
    nf_unregister_net_hook(&init_net, &nfho_rx_ipv6);
    nf_unregister_net_hook(&init_net, &nfho_tx_ipv6);

    printk(KERN_INFO "Packet sniffer module unloaded\n");
}

module_init(packet_sniffer_init);
module_exit(packet_sniffer_exit);

