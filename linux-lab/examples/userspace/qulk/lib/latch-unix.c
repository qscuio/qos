#include "latch.h"
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include "poll-loop.h"
#include "socket-util.h"

/* Initializes 'latch' as initially unset. */
void
latch_init(struct latch *latch)
{
    xpipe_nonblocking(latch->fds);
}

/* Destroys 'latch'. */
void
latch_destroy(struct latch *latch)
{
    close(latch->fds[0]);
    close(latch->fds[1]);
}

/* Resets 'latch' to the unset state.  Returns true if 'latch' was previously
 * set, false otherwise. */
bool
latch_poll(struct latch *latch)
{
    char latch_buffer[16];
    bool result = false;
    int ret;

    do {
        ret = read(latch->fds[0], &latch_buffer, sizeof latch_buffer);
        result |= ret > 0;
    /* Repeat as long as read() reads a full buffer. */
    } while (ret == sizeof latch_buffer);

    return result;
}

/* Sets 'latch'.
 *
 * Calls are not additive: a single latch_poll() clears out any number of
 * latch_set(). */
void
latch_set(struct latch *latch)
{
    ignore(write(latch->fds[1], "", 1));
}

/* Returns true if 'latch' is set, false otherwise.  Does not reset 'latch'
 * to the unset state. */
bool
latch_is_set(const struct latch *latch)
{
    struct pollfd pfd;
    int retval;

    pfd.fd = latch->fds[0];
    pfd.events = POLLIN;
    do {
        retval = poll(&pfd, 1, 0);
    } while (retval < 0 && errno == EINTR);

    return pfd.revents & POLLIN;
}

/* Causes the next poll_block() to wake up when 'latch' is set.
 *
 * ('where' is used in debug logging.  Commonly one would use latch_wait() to
 * automatically provide the caller's source file and line number for
 * 'where'.) */
void
latch_wait_at(const struct latch *latch, const char *where)
{
    poll_fd_wait_at(latch->fds[0], POLLIN, where);
}
