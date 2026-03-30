#ifndef __HAVE_DUMMY_H__
#define __HAVE_DUMMY_H__

#include <stdbool.h>

/* Degree of dummy support.
 *
 * Beyond enabling support for dummies, it can be useful to replace some kinds
 * of bridges and netdevs, or all kinds, by dummies.  This enum expresses the
 * degree to which this should happen. */
enum dummy_level {
    DUMMY_OVERRIDE_NONE,        /* Support dummy but don't force its use. */
    DUMMY_OVERRIDE_SYSTEM,      /* Replace "system" by dummy. */
    DUMMY_OVERRIDE_ALL,         /* Replace all types by dummy. */
};

/* For client programs to call directly to enable dummy support. */
void dummy_enable(const char *arg);

/* Implementation details. */
void dpif_dummy_register(enum dummy_level);
void netdev_dummy_register(enum dummy_level);
void timeval_dummy_register(void);
void ofpact_dummy_enable(void);

#endif /* __HAVE_DUMMY_H__ */
