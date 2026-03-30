#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define IOCTL_WRITE_MSG _IOW(240, 0, char *)

#define DEVICE_NAME "/dev/ulk_mmap"

int main() {
    int fd;
    char *mapped_mem;
    char message[] = "user space via ioctl!";

    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    mapped_mem = (char *)mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_mem == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Kernel memory mapped at address %p\n", mapped_mem);

    snprintf(mapped_mem, getpagesize(), "Hello from user space!\n");

    // Use ioctl to write a message to the kernel buffer
    if (ioctl(fd, IOCTL_WRITE_MSG, message) < 0) {
        perror("ioctl");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Kernel memory content: %s\n", mapped_mem);

    munmap(mapped_mem, getpagesize());

    close(fd);

    return EXIT_SUCCESS;
}

