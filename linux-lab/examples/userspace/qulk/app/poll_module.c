#define _XOPEN_SOURCE 700
#include <assert.h>
#include <fcntl.h> /* creat, O_CREAT */
#include <poll.h> /* poll */
#include <stdio.h> /* printf, puts, snprintf */
#include <stdlib.h> /* EXIT_FAILURE, EXIT_SUCCESS */
#include <unistd.h> /* read */

char *file_name = "/sys/kernel/debug/poll_module";

int main(int argc, char **argv) {
    char buf[1024];
    int fd, i, n;
    short revents;
    struct pollfd pfd;

    fd = open(file_name, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    pfd.fd = fd;
    pfd.events = POLLIN;
    while (1) {
        puts("poll");
        i = poll(&pfd, 1, -1);
        if (i == -1) {
            perror("poll");
            assert(0);
        }
        revents = pfd.revents;
        printf("revents = %d\n", revents);
        if (revents & POLLIN) {
            n = read(pfd.fd, buf, sizeof(buf));
            printf("POLLIN n=%d buf=%.*s\n", n, n, buf);
        }
    }
}
