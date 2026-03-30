#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int create_pidfd(pid_t pid) {
    int pidfd = syscall(SYS_pidfd_open, pid, 0);
    if (pidfd == -1) {
        perror("pidfd_open");
        exit(EXIT_FAILURE);
    }
    return pidfd;
}

void check_process_status(int pidfd) {
    struct stat statbuf;
    if (fstat(pidfd, &statbuf) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    printf("Process status checked successfully\n");
}

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
    int pidfd = create_pidfd(pid);

    check_process_status(pidfd);

    close(pidfd);
    return 0;
}

