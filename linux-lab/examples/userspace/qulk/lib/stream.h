#ifndef __HAVE_STREAM_H__
#define __HAVE_STREAM_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "types.h"
#include "socket-util.h"
#include "util.h"

struct pstream;
struct stream;
struct vlog_module;

void stream_usage(const char *name, bool active, bool passive, bool bootstrap);

/* Bidirectional byte streams. */
int stream_verify_name(const char *name);
int stream_open(const char *name, struct stream **, uint8_t dscp);
int stream_open_block(int error, long long int timeout, struct stream **);
void stream_close(struct stream *);
const char *stream_get_name(const struct stream *);
int stream_connect(struct stream *);
int stream_recv(struct stream *, void *buffer, size_t n);
int stream_send(struct stream *, const void *buffer, size_t n);

void stream_run(struct stream *);
void stream_run_wait(struct stream *);

enum stream_wait_type {
    STREAM_CONNECT,
    STREAM_RECV,
    STREAM_SEND
};
void stream_wait(struct stream *, enum stream_wait_type);
void stream_connect_wait(struct stream *);
void stream_recv_wait(struct stream *);
void stream_send_wait(struct stream *);
void stream_set_peer_id(struct stream *, const char *);
const char *stream_get_peer_id(const struct stream *);

/* Passive streams: listeners for incoming stream connections. */
int pstream_verify_name(const char *name);
int pstream_open(const char *name, struct pstream **, uint8_t dscp);
const char *pstream_get_name(const struct pstream *);
void pstream_close(struct pstream *);
int pstream_accept(struct pstream *, struct stream **);
int pstream_accept_block(struct pstream *, struct stream **);
void pstream_wait(struct pstream *);

ovs_be16 pstream_get_bound_port(const struct pstream *);

/* Convenience functions. */

int stream_open_with_default_port(const char *name,
                                  uint16_t default_port,
                                  struct stream **,
                                  uint8_t dscp);
int pstream_open_with_default_port(const char *name,
                                   uint16_t default_port,
                                   struct pstream **,
                                   uint8_t dscp);
bool stream_parse_target_with_default_port(const char *target,
                                           int default_port,
                                           struct sockaddr_storage *ss);
int stream_or_pstream_needs_probes(const char *name);

/* Error reporting. */

enum stream_content_type {
    STREAM_UNKNOWN,
    STREAM_OPENFLOW,
    STREAM_SSL,
    STREAM_JSONRPC
};

void stream_report_content(const void *, ssize_t, enum stream_content_type,
                           struct vlog_module *, const char *stream_name);


/* Stream replay helpers. */
void stream_replay_open_wfd(struct stream *, int open_result,
                            const char *name);
void pstream_replay_open_wfd(struct pstream *, int listen_result,
                             const char *name);
void stream_replay_close_wfd(struct stream *);
void pstream_replay_close_wfd(struct pstream *);
void stream_replay_write(struct stream *, const void *, int, bool is_read);
void pstream_replay_write_accept(struct pstream *, const struct stream *,
                                 int accept_result);

#endif /* __HAVE_STREAM_H__ */
