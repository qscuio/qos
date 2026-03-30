#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/tipc.h>

#define MAXEVENTS 128

static void die(const char *s) {
    perror(s);
    exit(-1);
}

int main(int argc, char *argv[]) {
    struct sockaddr_tipc listen_sa = {.family             = AF_TIPC,
                                      .addrtype           = TIPC_ADDR_NAMESEQ,
                                      .addr.nameseq.type  = 4321,
                                      .addr.nameseq.lower = 0,
                                      .addr.nameseq.upper = ~0,
                                      .scope              = TIPC_ZONE_SCOPE};
    int                  listen_fd, peer_fd, efd, i, n;
    struct epoll_event   evt;
    struct epoll_event  *events;

    listen_fd = socket(AF_TIPC, SOCK_STREAM, 0);
    if (listen_fd <= 0)
        die("socket");
    if (bind(listen_fd, (struct sockaddr *) &listen_sa, sizeof(listen_sa)))
        die("bind");
    if (listen(listen_fd, 0))
        die("listen");
    if (fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL, 0) | O_NONBLOCK) <
        0)
        die("fcntl");

    efd = epoll_create1(0);
    if (efd == -1)
        die("epoll_create1");
    evt.data.fd = listen_fd;
    evt.events  = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &evt))
        die("epoll_ctl");
    events = calloc(MAXEVENTS, sizeof(struct epoll_event));

    fprintf(stderr,
            "Server listening for stream connections on {4321,0, ~0}\n");
    while (1) {
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[ i ].events & EPOLLHUP) &&
                (!(events[ i ].events & EPOLLIN))) {
                /*FD error*/
                fprintf(stderr, "POLLHUP on socket %d\n", events[ i ].data.fd);
                close(events[ i ].data.fd);
                continue;
            }
            /*Event on server listen sock*/
            else if (listen_fd == events[ i ].data.fd) {
            do_accept:
                peer_fd = accept(listen_fd, 0, 0);
                if (peer_fd > 0) {
                    if (fcntl(peer_fd, F_SETFL,
                              fcntl(peer_fd, F_GETFL, 0) | O_NONBLOCK) < 0)
                        die("fcntl");
                    evt.data.fd = peer_fd;
                    evt.events  = EPOLLIN | EPOLLET;
                    fprintf(stderr, "Accepted new client connection %d\n",
                            peer_fd);
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, peer_fd, &evt))
                        die("epoll_ctl");
                    goto do_accept;
                }
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    fprintf(stderr, "No more events on listening sock\n");
                    continue;
                }
            } else {
                ssize_t count;
                char    buf[ 512 ];
            do_read:
                count = read(events[ i ].data.fd, buf, sizeof(buf));
                if (count == -1) {
                    if (errno == EAGAIN) {
                        perror("No more data to read");
                        continue;
                    } else if (errno == ECONNRESET) {
                        perror("read()");
                        goto do_close;
                    } else
                        die("read()");
                }
                if (count == 0) {
                do_close:
                    fprintf(stderr, "Peer have closed the connection\n");
                    close(events[ i ].data.fd);
                    continue;
                }
                if (write(1, buf, count) < 0)
                    die("write");
                goto do_read;
            }
        }
    }
    return 0;
}
