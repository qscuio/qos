#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 4096

int main() {
    int fd;
    char buf[BUFFER_SIZE];

    fd = open("/dev/memory_alloc", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open the device...");
        return errno;
    }

    ssize_t num_bytes = read(fd, buf, BUFFER_SIZE - 1);
    if (num_bytes < 0) {
        perror("Failed to read from the device...");
        close(fd);
        return errno;
    }

    buf[num_bytes] = '\0'; // Ensure null-terminated string
    printf("Received message from kernel: %s\n", buf);

    close(fd);
    return 0;
}
