#include "stream.h"
#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ovs-atomic.h"
#include "poll-loop.h"
#include "socket-util.h"
#include "dirs.h"
#include "util.h"
#include "stream-provider.h"
#include "stream-fd.h"
#include "vlog.h"

VLOG_DEFINE_THIS_MODULE(stream_unix);

/* Active UNIX socket. */

static int
unix_open(const char *name, char *suffix, struct stream **streamp,
          uint8_t dscp OVS_UNUSED)
{
    char *connect_path;
    int fd;

#if 0 /* liwei */
    connect_path = abs_file_name(ovs_rundir(), suffix);
#else
    connect_path = abs_file_name(".", suffix);
#endif
    fd = make_unix_socket(SOCK_STREAM, true, NULL, connect_path);

    if (fd < 0) {
        VLOG_DBG("%s: connection failed (%s)",
                 connect_path, ovs_strerror(-fd));
        free(connect_path);
        return -fd;
    }

    free(connect_path);
    return new_fd_stream(xstrdup(name), fd, check_connection_completion(fd),
                         AF_UNIX, streamp);
}

const struct stream_class unix_stream_class = {
    "unix",                     /* name */
    false,                      /* needs_probes */
    unix_open,                  /* open */
    NULL,                       /* close */
    NULL,                       /* connect */
    NULL,                       /* recv */
    NULL,                       /* send */
    NULL,                       /* run */
    NULL,                       /* run_wait */
    NULL,                       /* wait */
};

/* Passive UNIX socket. */

static int punix_accept(int fd, const struct sockaddr_storage *ss,
                        size_t ss_len, struct stream **streamp);

static int
punix_open(const char *name OVS_UNUSED, char *suffix,
           struct pstream **pstreamp, uint8_t dscp OVS_UNUSED)
{
    char *bind_path;
    int fd, error;

#if 0 /* liwei */
    bind_path = abs_file_name(ovs_rundir(), suffix);
#else
    bind_path = abs_file_name(".", suffix);
#endif
    fd = make_unix_socket(SOCK_STREAM, true, bind_path, NULL);
    if (fd < 0) {
        VLOG_ERR("%s: binding failed: %s", bind_path, ovs_strerror(errno));
        free(bind_path);
        return errno;
    }

    if (listen(fd, 64) < 0) {
        error = errno;
        VLOG_ERR("%s: listen: %s", name, ovs_strerror(error));
        close(fd);
        free(bind_path);
        return error;
    }

    return new_fd_pstream(xstrdup(name), fd,
                          punix_accept, bind_path, pstreamp);
}

static int
punix_accept(int fd, const struct sockaddr_storage *ss, size_t ss_len,
             struct stream **streamp)
{
    const struct sockaddr_un *sun = (const struct sockaddr_un *) ss;
    int name_len = get_unix_name_len(sun, ss_len);
    char *bound_name;

    if (name_len > 0) {
        bound_name = xasprintf("unix:%.*s", name_len, sun->sun_path);
    } else {
        /* When a Unix socket connects to us without first binding a name, we
         * don't get any name for it.  It's useful nevertheless to be able to
         * distinguish separate sockets in log messages, so use a counter. */
        static atomic_count next_idx = ATOMIC_COUNT_INIT(0);
        bound_name = xasprintf("unix#%u", atomic_count_inc(&next_idx));
    }
    return new_fd_stream(bound_name, fd, 0, AF_UNIX, streamp);
}

const struct pstream_class punix_pstream_class = {
    "punix",
    false,
    punix_open,
    NULL,
    NULL,
    NULL,
};

