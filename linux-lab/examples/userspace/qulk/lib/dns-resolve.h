#ifndef __HAVE_DNS_RESOLVE_H__
#define __HAVE_DNS_RESOLVE_H__

#include <stdbool.h>

void dns_resolve_init(bool is_daemon);
bool dns_resolve(const char *name, char **addr);
void dns_resolve_destroy(void);

#endif /* __HAVE_DNS_RESOLVE_H__ */
