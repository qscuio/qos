#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>

static int m_uts_init(void)
{
    struct new_utsname *uts = init_utsname();
    pr_info(
	    "sysname    = %s\n"
	    "nodename   = %s\n"
	    "release    = %s\n"
	    "version    = %s\n"
	    "machine    = %s\n"
	    "domainname = %s\n",
	    uts->sysname,
	    uts->nodename,
	    uts->release,
	    uts->version,
	    uts->machine,
	    uts->domainname
	   );

    /* Nice try, but it is not a member. */
    /*pr_info("THIS_MODULE->vermagic = %s\n", THIS_MODULE->vermagic);*/
    return 0;
}

static void m_uts_exit(void) {}

module_init(m_uts_init)
module_exit(m_uts_exit)
MODULE_LICENSE("GPL");
