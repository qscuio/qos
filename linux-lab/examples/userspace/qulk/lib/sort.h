#ifndef __HAVE_SORT_H__
#define __HAVE_SORT_H__

#include <stddef.h>

void sort(size_t count,
          int (*compare)(size_t a, size_t b, void *aux),
          void (*swap)(size_t a, size_t b, void *aux),
          void *aux);

#endif /* __HAVE_SORT_H__ */
