#ifndef __HAVE_LATCH_H__
#define __HAVE_LATCH_H__

/* A thread-safe, signal-safe, pollable doorbell.
 *
 * This is a thin wrapper around a pipe that allows threads to notify each
 * other that an event has occurred in a signal-safe way  */

#include <stdbool.h>
#include "util.h"

struct latch {
    int fds[2];
};

void latch_init(struct latch *);
void latch_destroy(struct latch *);

bool latch_poll(struct latch *);
void latch_set(struct latch *);

bool latch_is_set(const struct latch *);
void latch_wait_at(const struct latch *, const char *where);
#define latch_wait(latch) latch_wait_at(latch, OVS_SOURCE_LOCATOR)

#endif /* __HAVE_LATCH_H__ */
