#ifndef __HAVE_DIRS_H__
#define __HAVE_DIRS_H__

const char *ovs_sysconfdir(void); /* /usr/local/etc */
const char *ovs_pkgdatadir(void); /* /usr/local/share/openvswitch */
const char *ovs_rundir(void);     /* /usr/local/var/run/openvswitch */
const char *ovs_logdir(void);     /* /usr/local/var/log/openvswitch */
const char *ovs_dbdir(void);      /* /usr/local/etc/openvswitch */
const char *ovs_bindir(void);     /* /usr/local/bin */

#endif /* __HAVE_DIRS_H__ */
