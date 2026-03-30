#ifndef __HAVE_SKIPLIST_H__
#define __HAVE_SKIPLIST_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef int (skiplist_comparator)(const void *a, const void *b,
                                  const void *conf);

struct skiplist_node;

struct skiplist;

#define SKIPLIST_FOR_EACH (SKIPLIST_NODE, SKIPLIST) \
    for (SKIPLIST_NODE = skiplist_first(SKIPLIST); \
         SKIPLIST_NODE; \
         SKIPLIST_NODE = skiplist_next(SKIPLIST_NODE))

struct skiplist *skiplist_create(skiplist_comparator *object_comparator,
                                 void *configuration);
void skiplist_insert(struct skiplist *sl, const void *object);
void *skiplist_delete(struct skiplist *sl, const void *object);
struct skiplist_node *skiplist_find(struct skiplist *sl, const void *value);
void *skiplist_get_data(struct skiplist_node *node);
uint32_t skiplist_get_size(struct skiplist *sl);
struct skiplist_node *skiplist_forward_to(struct skiplist *sl,
                                          const void *value);
struct skiplist_node *skiplist_first(struct skiplist *sl);
struct skiplist_node *skiplist_next(struct skiplist_node *node);
void skiplist_destroy(struct skiplist *sl, void (*func)(void *));

#endif /* __HAVE_SKIPLIST_H__ */
