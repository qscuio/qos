#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/chardev_example"
#define BUFFER_SIZE 1024

int main() {
    int fd;
    char read_buf[BUFFER_SIZE];
    char write_buf[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    // Open the character device file
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device file");
        return EXIT_FAILURE;
    }

    // Example: Writing to the device
    sprintf(write_buf, "Hello from user space!\n");
    bytes_written = write(fd, write_buf, strlen(write_buf));
    if (bytes_written < 0) {
        perror("Failed to write to the device");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Wrote %zd bytes to the device\n", bytes_written);

    // Example: Reading from the device
    bytes_read = read(fd, read_buf, sizeof(read_buf) - 1);
    if (bytes_read < 0) {
        perror("Failed to read from the device");
        close(fd);
        return EXIT_FAILURE;
    }
    read_buf[bytes_read] = '\0';
    printf("Read %zd bytes from the device: %s\n", bytes_read, read_buf);

    // Close the character device file
    close(fd);

    return EXIT_SUCCESS;
}

