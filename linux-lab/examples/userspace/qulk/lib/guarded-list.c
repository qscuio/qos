#include "guarded-list.h"

void
guarded_list_init(struct guarded_list *list)
{
    ovs_mutex_init(&list->mutex);
    ovs_list_init(&list->list);
    list->n = 0;
}

void
guarded_list_destroy(struct guarded_list *list)
{
    ovs_mutex_destroy(&list->mutex);
}

bool
guarded_list_is_empty(const struct guarded_list *list)
{
    bool empty;

    ovs_mutex_lock(&list->mutex);
    empty = list->n == 0;
    ovs_mutex_unlock(&list->mutex);

    return empty;
}

/* If 'list' has fewer than 'max' elements, adds 'node' at the end of the list
 * and returns the number of elements now on the list.
 *
 * If 'list' already has at least 'max' elements, returns 0 without modifying
 * the list. */
size_t
guarded_list_push_back(struct guarded_list *list,
                       struct ovs_list *node, size_t max)
{
    size_t retval = 0;

    ovs_mutex_lock(&list->mutex);
    if (list->n < max) {
        ovs_list_push_back(&list->list, node);
        retval = ++list->n;
    }
    ovs_mutex_unlock(&list->mutex);

    return retval;
}

struct ovs_list *
guarded_list_pop_front(struct guarded_list *list)
{
    struct ovs_list *node = NULL;

    ovs_mutex_lock(&list->mutex);
    if (list->n) {
        node = ovs_list_pop_front(&list->list);
        list->n--;
    }
    ovs_mutex_unlock(&list->mutex);

    return node;
}

size_t
guarded_list_pop_all(struct guarded_list *list, struct ovs_list *elements)
{
    size_t n;

    ovs_mutex_lock(&list->mutex);
    ovs_list_move(elements, &list->list);
    n = list->n;

    ovs_list_init(&list->list);
    list->n = 0;
    ovs_mutex_unlock(&list->mutex);

    return n;
}
