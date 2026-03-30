#ifndef __HAVE_STREAM_FD_H__
#define __HAVE_STREAM_FD_H__ 

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct stream;
struct pstream;
struct sockaddr_storage;

int new_fd_stream(char *name, int fd, int connect_status,
                  int fd_type, struct stream **streamp);
int new_fd_pstream(char *name, int fd,
                   int (*accept_cb)(int fd, const struct sockaddr_storage *ss,
                                    size_t ss_len, struct stream **),
                   char *unlink_path,
                   struct pstream **pstreamp);

#endif /* __HAVE_STREAM_FD_H__ */
