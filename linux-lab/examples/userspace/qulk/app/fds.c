#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/inotify.h>
#include <signal.h>
#include <string.h>

#define MAX_EVENTS 10

void handle_eventfd(int fd) {
    uint64_t value;
    read(fd, &value, sizeof(value));
    printf("eventfd triggered with value: %lu\n", value);
}

void handle_timerfd(int fd) {
    uint64_t expirations;
    read(fd, &expirations, sizeof(expirations));
    printf("timerfd expired %lu times\n", expirations);
}

void handle_signalfd(int fd) {
    struct signalfd_siginfo fdsi;
    read(fd, &fdsi, sizeof(fdsi));
    printf("signal received: %d\n", fdsi.ssi_signo);
}

void handle_inotify(int fd) {
    char buffer[1024];
    int length = read(fd, buffer, 1024);
    if (length < 0) {
        perror("read");
    }
    struct inotify_event *event = (struct inotify_event *) &buffer[0];
    if (event->len) {
        if (event->mask & IN_CREATE) {
            printf("The file %s was created.\n", event->name);
        }
        if (event->mask & IN_DELETE) {
            printf("The file %s was deleted.\n", event->name);
        }
    }
}

int main() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // eventfd
    int efd = eventfd(0, 0);
    if (efd == -1) {
        perror("eventfd");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = efd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, efd, &ev) == -1) {
        perror("epoll_ctl: eventfd");
        exit(EXIT_FAILURE);
    }

    // timerfd
    int tfd = timerfd_create(CLOCK_REALTIME, 0);
    if (tfd == -1) {
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    struct itimerspec new_value;
    new_value.it_value.tv_sec = 2;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 2;
    new_value.it_interval.tv_nsec = 0;
    if (timerfd_settime(tfd, 0, &new_value, NULL) == -1) {
        perror("timerfd_settime");
        exit(EXIT_FAILURE);
    }

    ev.data.fd = tfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
        perror("epoll_ctl: timerfd");
        exit(EXIT_FAILURE);
    }

    // signalfd
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }

    ev.data.fd = sfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
        perror("epoll_ctl: signalfd");
        exit(EXIT_FAILURE);
    }

    // inotify
    int ifd = inotify_init();
    if (ifd == -1) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    int wd = inotify_add_watch(ifd, ".", IN_CREATE | IN_DELETE);
    if (wd == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    ev.data.fd = ifd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ifd, &ev) == -1) {
        perror("epoll_ctl: inotify");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == efd) {
                handle_eventfd(efd);
            } else if (events[n].data.fd == tfd) {
                handle_timerfd(tfd);
            } else if (events[n].data.fd == sfd) {
                handle_signalfd(sfd);
            } else if (events[n].data.fd == ifd) {
                handle_inotify(ifd);
            }
        }
    }

    close(efd);
    close(tfd);
    close(sfd);
    close(ifd);
    close(epoll_fd);
    return 0;
}

