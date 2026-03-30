#define _GNU_SOURCE
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_EVENTS 10

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        sleep(5);
        exit(EXIT_SUCCESS);
    }

    // Parent process
    int pidfd = syscall(SYS_pidfd_open, pid, 0);
    if (pidfd == -1) {
        perror("pidfd_open");
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = pidfd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pidfd, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for process to terminate...\n");

    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
        if (events[n].data.fd == pidfd) {
            printf("Process %d terminated.\n", pid);
        }
    }

    close(pidfd);
    close(epoll_fd);

    return 0;
}

