#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define DEVICE_NAME "/dev/fasync_example"

static void sigio_handler(int signum) {
    printf("Received SIGIO signal\n");
}

int main() {
    int fd;
    char buffer[128];
    struct sigaction sa;

    sa.sa_handler = sigio_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGIO, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    fd = open(DEVICE_NAME, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd, F_SETOWN, getpid()) == -1) {
        perror("fcntl F_SETOWN");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd, F_SETFL, FASYNC) == -1) {
        perror("fcntl F_SETFL");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Writing to device...\n");
    if (write(fd, "Hello, world!", 13) == -1) {
        perror("write");
    }

    printf("Reading from device...\n");
    if (read(fd, buffer, sizeof(buffer)) == -1) {
        perror("read");
    } else {
        printf("Read from device: %s\n", buffer);
    }

    close(fd);
    return 0;
}

