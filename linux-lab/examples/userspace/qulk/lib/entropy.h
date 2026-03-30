#ifndef __HAVE_ENTROPY_H__
#define __HAVE_ENTROPY_H__

#include <stddef.h>

int get_entropy(void *, size_t);
void get_entropy_or_die(void *, size_t);

#endif /* __HAVE_ENTROPY_H__ */
