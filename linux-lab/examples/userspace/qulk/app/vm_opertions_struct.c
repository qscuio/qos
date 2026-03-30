#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    char *mapped = NULL;
    char *mapped1 = NULL;
    char *mapped2 = NULL;

    char command[128] = {0};

    int fd = open("/dev/vm_ops_example", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    mapped = (char *)mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    mapped1 = (char *)mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped1 == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    mapped2 = (char *)mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped2 == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    sprintf(command, "cat /proc/%d/maps", getpid());
    system(command);
    printf("Read from mmap: %s\n", mapped);

    sleep(3);
    munmap(mapped, 4096);
    close(fd);
    return 0;
}

