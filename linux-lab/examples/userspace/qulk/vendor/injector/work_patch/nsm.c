#include "pal.h"
#include "lib.h"
#include "if.h"
#include "cli.h"
#include "cli_mode.h"
#include "sockunion.h"
#include "prefix.h"
#include "log.h"
#include "table.h"
#include "thread.h"
#include "nsm_message.h"
#include "nsm_client.h"
#include "pal/linux/pal_dns.h"
#include "ptree.h"
#include "bitmap.h"
#include "hash.h"
#include "avl_tree.h"
#include "lib_mtrace.h"
#include "nsmd.h"
#include "nsm_vrf_snmp.h"
#include "vty.h"
#include "nsm_snmp.h"
#include "parser_utils.h"
#include <dlfcn.h>
//#include "nsm_interface.h"

int all_done = 0;
int sock_pair[2]; 
extern struct lib_globals *nzg;

static void (*_nsm_if_refresh) (struct interface *);
static int (*_nsm_if_flag_up_set) (uint32_t vr_id, char *ifname, int iterate_members);
static int (*_nsm_if_flag_up_unset) (uint32_t vr_id, char *ifname, int iterate_members);

void foo(void)
{
    zlog_info(nzg, "[%s][%d]------\n", __func__, __LINE__);
}

int local_timer(struct thread *thread)
{
    static int i = 0;
    struct ipi_vr *vr = NULL;
    struct interface *ifp = NULL;

    i++;
    zlog_info(nzg, "Hello from timer in %s, %d\n", nzg->progname, i);

    vr = LIB_GLOB_GET_VR_CONTEXT (nzg);
    if (!vr)
    {
        zlog_err(nzg, "Failed to find default vr\n");
        return -1;
    }


    ifp = if_lookup_by_name(&vr->ifm, "vlan1.1");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface vlan1.1 %p\n", ifp);
        _nsm_if_refresh(ifp);
        _nsm_if_flag_up_set(vr->id, "vlan1.1", 0);
        _nsm_if_refresh(ifp);
        _nsm_if_flag_up_unset(vr->id, "vlan1.1", 0);
    }

    ifp = if_lookup_by_name(&vr->ifm, "eth4/2");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface eth4/2 %p\n", ifp);
        _nsm_if_refresh(ifp);
        _nsm_if_flag_up_set(vr->id, "eth4/2", 0);
        _nsm_if_refresh(ifp);
        _nsm_if_flag_up_unset(vr->id, "eth4/2", 0);
    }

    if (i < 60)
    {
        thread_add_timer(nzg, local_timer, NULL, 1);
        return 0;
    }

    all_done = 1;

    return 0;
}

int local_event(struct thread *thread)
{
    zlog_info(nzg, "Hello from event in %s\n", nzg->progname);

    thread_add_timer(nzg, local_timer, NULL, 1);
    return 0;
}

void get_handle_of_current_library() {
    Dl_info dl_info;
    if (dladdr((void*)get_handle_of_current_library, &dl_info) == 0) {
        fprintf(stderr, "Error: dladdr failed\n");
        exit(EXIT_FAILURE);
    }

    _nsm_if_refresh = dlsym(NULL, "nsm_if_refresh");
    if (!_nsm_if_refresh)
    {
        fprintf(stderr, "Error: cannot find _nsm_if_refresh\n");
        exit(EXIT_FAILURE);
    }

    _nsm_if_flag_up_set = dlsym(NULL, "nsm_if_flag_up_set");
    if (!_nsm_if_flag_up_set)
    {
        fprintf(stderr, "Error: cannot find _nsm_if_flag_up\n");
        exit(EXIT_FAILURE);
    }

    _nsm_if_flag_up_unset = dlsym(NULL, "nsm_if_flag_up_unset");
    if (!_nsm_if_flag_up_unset)
    {
        fprintf(stderr, "Error: cannot find nsm_if_flag_up_unset\n");
        exit(EXIT_FAILURE);
    }
#if 0
    void* handle = dlopen(dl_info.dli_fname, RTLD_LAZY | RTLD_NOLOAD);
    if (!handle) {
        fprintf(stderr, "Error: dlopen failed: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }
#endif
}

int do_basic_setup(void)
{
    int ret = 0;

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sock_pair);
    if (ret == -1)
    {
        zlog_info(nzg, "cannot create done channel\n");
        return -1;
    }

    return 0;
}

__attribute((constructor)) void init(void)
{
    struct ipi_vr *vr = NULL;
    struct interface *ifp = NULL;

    get_handle_of_current_library();

    zlog_info(nzg, "Hello to the %s world\n", nzg->progname);

    vr = LIB_GLOB_GET_VR_CONTEXT (nzg);
    if (!vr)
    {
        zlog_err(nzg, "Failed to find default vr\n");
        return;
    }
    else
    {
        zlog_info(nzg, "Successfully find vr %p\n", vr);
    }

    ifp = if_lookup_by_name(&vr->ifm, "mgmt");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface mgmt %p\n", ifp);
    }
    else
    {
        zlog_err(nzg, "Cannot find interface mgmt");
    }

    ifp = if_lookup_by_name(&vr->ifm, "vlan1.1");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface vlan1.1 %p\n", ifp);
    }
    else
    {
        zlog_err(nzg, "Cannot find interface vlan1.1");
    }

    ifp = if_lookup_by_name(&vr->ifm, "vlan1.100");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface vlan1.100 %p\n", ifp);
    }
    else
    {
        zlog_err(nzg, "Cannot find interface vlan1.100");
    }

    ifp = if_lookup_by_name(&vr->ifm, "vlan1.200");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface vlan1.200 %p\n", ifp);
    }
    else
    {
        zlog_err(nzg, "Cannot find interface vlan1.200");
    }

    ifp = if_lookup_by_name(&vr->ifm, "vlan1.300");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface vlan1.300 %p\n", ifp);
    }
    else
    {
        zlog_err(nzg, "Cannot find interface vlan1.300");
    }

#if 0
    ifp = if_lookup_by_name(&vr->ifm, "vlan1");
    if (ifp)
    {
        zlog_info(nzg, "Successfully find interface vlan1 %p\n", ifp);
    }
    else
    {
        zlog_err(nzg, "Cannot find interface vlan1\n");
    }
#endif

    thread_add_event(nzg, local_event, NULL, 1);

    return;
}

__attribute((destructor)) void deinit(void)
{
    int done = 0;
    struct thread thread;

    while (!all_done && thread_fetch(nzg, &thread))
    {
        thread_call (&thread);
    }

    zlog_info(nzg, "Goodby from the %s world\n", nzg->progname);
}
