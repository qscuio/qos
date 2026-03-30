#define _GNU_SOURCE
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_EVENTS 10

int create_pidfd(pid_t pid) {
    int pidfd = syscall(SYS_pidfd_open, pid, 0);
    if (pidfd == -1) {
        perror("pidfd_open");
        exit(EXIT_FAILURE);
    }
    return pidfd;
}

void monitor_processes(int epoll_fd, int pidfds[], int count) {
    struct epoll_event events[MAX_EVENTS];

    printf("Waiting for processes to terminate...\n");

    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
        for (int i = 0; i < count; ++i) {
            if (events[n].data.fd == pidfds[i]) {
                printf("Process with pidfd %d terminated.\n", pidfds[i]);
                close(pidfds[i]);
            }
        }
    }
}

int main() {
    pid_t pids[2];
    int pidfds[2];

    for (int i = 0; i < 2; ++i) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pids[i] == 0) {
            sleep(5 + i);
            exit(EXIT_SUCCESS);
        }
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 2; ++i) {
        pidfds[i] = create_pidfd(pids[i]);

        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = pidfds[i];

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pidfds[i], &event) == -1) {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }
    }

    monitor_processes(epoll_fd, pidfds, 2);

    close(epoll_fd);
    return 0;
}

