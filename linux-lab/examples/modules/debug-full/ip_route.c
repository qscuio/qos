#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <net/route.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple example of using ip_route_output_key");
MODULE_VERSION("1.0");

static int ___ip_route_lookup(char *dst, char *src)
{
    struct rtable *rt;
    struct flowi4 fl4;
    struct net *net = &init_net;
    struct in_addr dst_ip;
    struct in_addr src_ip;
    
    dst_ip.s_addr = in_aton(dst);
    src_ip.s_addr = in_aton(src);

    memset(&fl4, 0, sizeof(fl4));
    fl4.daddr = dst_ip.s_addr;
    fl4.saddr = src_ip.s_addr;
    fl4.flowi4_oif = 0;
    fl4.flowi4_mark = 0;
    fl4.flowi4_flags = 0;
    fl4.flowi4_tos = 0;

    rt = ip_route_output_key(net, &fl4);
    if (IS_ERR(rt)) {
        printk(KERN_INFO "Route lookup failed for dst %s src %s\n", dst, src);
        return -1;
    }

    printk(KERN_INFO "Route lookup successful: destination IP = %pI4, src %s dev %s\n", &dst_ip, src, rt->dst.dev->name);

    ip_rt_put(rt);

    return 0;

}
static int __init my_module_init(void) {
    ___ip_route_lookup("8.8.8.8", "0.0.0.0");
    ___ip_route_lookup("8.8.8.8", "224.0.0.0");
    ___ip_route_lookup("8.8.8.8", "255.255.255.255");
    ___ip_route_lookup("255.255.255.255", "0.0.0.0");
    ___ip_route_lookup("8.8.8.8", "10.10.10.1");
    ___ip_route_lookup("8.8.8.8", "10.10.10.20");
    ___ip_route_lookup("8.8.8.8", "10.10.10.254");
    ___ip_route_lookup("8.8.8.8", "10.20.20.1");
    ___ip_route_lookup("8.8.8.8", "10.20.20.20");
    ___ip_route_lookup("8.8.8.8", "10.20.20.254");
    ___ip_route_lookup("10.10.10.100", "0.0.0.0");
    ___ip_route_lookup("10.10.10.100", "255.255.255.0");
    ___ip_route_lookup("10.10.10.100", "8.8.8.8");
    ___ip_route_lookup("10.10.10.100", "10.10.10.1");
    ___ip_route_lookup("10.10.10.100", "10.10.10.254");
    ___ip_route_lookup("10.10.10.100", "10.20.20.1");
    ___ip_route_lookup("10.10.10.100", "10.20.20.254");
    ___ip_route_lookup("100.73.26.131", "100.73.24.1");

    return 0;
}

static void __exit my_module_exit(void) {
    printk(KERN_INFO "Exiting module\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

