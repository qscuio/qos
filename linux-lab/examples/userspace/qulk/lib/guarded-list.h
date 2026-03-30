#ifndef __HAVE_GUARDED_LIST_H__
#define __HAVE_GUARDED_LIST_H__

#include <stddef.h>
#include "compiler.h"
#include "list.h"
#include "ovs-thread.h"

struct guarded_list {
    struct ovs_mutex mutex;
    struct ovs_list list;
    size_t n;
};

#define GUARDED_OVS_LIST_INITIALIZER(LIST) { \
    .mutex = OVS_MUTEX_INITIALIZER, \
    .list = OVS_LIST_INITIALIZER(&((LIST)->list)), \
    .n = 0 }

void guarded_list_init(struct guarded_list *);
void guarded_list_destroy(struct guarded_list *);

bool guarded_list_is_empty(const struct guarded_list *);

size_t guarded_list_push_back(struct guarded_list *, struct ovs_list *,
                              size_t max);
struct ovs_list *guarded_list_pop_front(struct guarded_list *);
size_t guarded_list_pop_all(struct guarded_list *, struct ovs_list *);

#endif /* __HAVE_GUARDED_LIST_H__ */
