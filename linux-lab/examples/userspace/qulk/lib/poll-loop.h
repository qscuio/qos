/* High-level wrapper around the "poll" system call.
 *
 * The intended usage is for each thread's main loop to go about its business
 * servicing whatever events it needs to.  Then, when it runs out of immediate
 * tasks, it calls each subordinate module's "wait" function, which in turn
 * calls one (or more) of the functions poll_fd_wait(), poll_immediate_wake(),
 * and poll_timer_wait() to register to be awakened when the appropriate event
 * occurs.  Then the main loop calls poll_block(), which blocks until one of
 * the registered events happens.
 *
 *
 * Thread-safety
 * =============
 *
 * The poll set is per-thread, so all functions in this module are thread-safe.
 */
#ifndef __HAVE_POLL_LOOP_H__
#define __HAVE_POLL_LOOP_H__

#include <poll.h>

/* Schedule events to wake up the following poll_block().
 *
 * The poll_loop logs the 'where' argument to each function at "debug" level
 * when an event causes a wakeup.  Each of these ways to schedule an event has
 * a function and a macro wrapper.  The macro version automatically supplies
 * the source code location of the caller.  The function version allows the
 * caller to supply a location explicitly, which is useful if the caller's own
 * caller would be more useful in log output.  See timer_wait_at() for an
 * example. */
void poll_fd_wait_at(int fd, short int events, const char *where);
#define poll_fd_wait(fd, events) poll_fd_wait_at(fd, events, OVS_SOURCE_LOCATOR)

void poll_timer_wait_at(long long int msec, const char *where);
#define poll_timer_wait(msec) poll_timer_wait_at(msec, OVS_SOURCE_LOCATOR)

void poll_timer_wait_until_at(long long int msec, const char *where);
#define poll_timer_wait_until(msec)             \
    poll_timer_wait_until_at(msec, OVS_SOURCE_LOCATOR)

void poll_immediate_wake_at(const char *where);
#define poll_immediate_wake() poll_immediate_wake_at(OVS_SOURCE_LOCATOR)

/* Wait until an event occurs. */
void poll_block(void);

#endif /* __HAVE_POLL_LOOP_H__ */
