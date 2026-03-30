#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int create_pidfd(pid_t pid) {
    int pidfd = syscall(SYS_pidfd_open, pid, 0);
    if (pidfd == -1) {
        perror("pidfd_open");
        exit(EXIT_FAILURE);
    }
    return pidfd;
}

void send_signal(int pidfd, int signal) {
    if (syscall(SYS_pidfd_send_signal, pidfd, signal, NULL, 0) == -1) {
        perror("pidfd_send_signal");
        exit(EXIT_FAILURE);
    }
}

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        while (1) {
            printf("Child process running...\n");
            sleep(1);
        }
    }

    // Parent process
    int pidfd = create_pidfd(pid);

    sleep(5); // Let the child run for a while

    printf("Sending SIGTERM to child process...\n");
    send_signal(pidfd, SIGTERM);

    close(pidfd);
    return 0;
}

