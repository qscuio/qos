// controller.c
#define _GNU_SOURCE
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main() {
    pid_t target_pid;
    void* target_address;
    char buffer[100];
    struct iovec local_iov;
    struct iovec remote_iov;
    ssize_t nread, nwrite;

    printf("Enter target process PID: ");
    scanf("%d", &target_pid);

    printf("Enter target buffer address (in hex): ");
    scanf("%p", &target_address);

    // Read from the target process
    local_iov.iov_base = buffer;
    local_iov.iov_len = sizeof(buffer);
    remote_iov.iov_base = target_address;
    remote_iov.iov_len = sizeof(buffer);

    nread = process_vm_readv(target_pid, &local_iov, 1, &remote_iov, 1, 0);
    if (nread == -1) {
        perror("process_vm_readv");
        return 1;
    }

    printf("Read %zd bytes from target process: ", nread);
    for (int i = 0; i < nread; i++) {
        printf("%02x ", (unsigned char)buffer[i]);
    }
    printf("\n");

    // Modify the buffer
    strcpy(buffer, "Hello from the controller!");

    // Write to the target process
    nwrite = process_vm_writev(target_pid, &local_iov, 1, &remote_iov, 1, 0);
    if (nwrite == -1) {
        perror("process_vm_writev");
        return 1;
    }

    printf("Wrote %zd bytes to target process\n", nwrite);

    return 0;
}

