#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/icmp.h>

// Netfilter hook options
static struct nf_hook_ops nfho;

// Hook function to inspect packets
static unsigned int hook_func(void *priv,
                              struct sk_buff *skb,
                              const struct nf_hook_state *state) {
    struct iphdr *ip_header;
    struct icmphdr *icmp_header;

    // Check if the packet is an IP packet
    ip_header = ip_hdr(skb);
    if (!ip_header) return NF_ACCEPT;

    // Check if the packet is an ICMP packet
    if (ip_header->protocol == IPPROTO_ICMP) {
        icmp_header = icmp_hdr(skb);
        if (icmp_header) {
            printk(KERN_INFO "Dropping ICMP packet\n");
            return NF_DROP;
        }
    }

    return NF_ACCEPT;
}

// Module initialization
static int __init mymodule_init(void) {
    nfho.hook = hook_func;             // Hook function
    nfho.hooknum = NF_INET_PRE_ROUTING;// Hook in the IP pre-routing stage
    nfho.pf = PF_INET;                 // IPv4 protocol
    nfho.priority = NF_IP_PRI_FIRST;   // Highest priority

    nf_register_net_hook(&init_net, &nfho); // Register the hook

    printk(KERN_INFO "Netfilter module loaded\n");
    return 0;
}

// Module cleanup
static void __exit mymodule_exit(void) {
    nf_unregister_net_hook(&init_net, &nfho); // Unregister the hook
    printk(KERN_INFO "Netfilter module unloaded\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Netfilter module to drop ICMP packets");

