#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define BUFFER_SIZE 256

int main() {
    int sv[2];  // Socket pair array
    pid_t pid;
    char buffer[BUFFER_SIZE];

    // Create a socket pair
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    pid = fork();  // Fork a child process

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  // Child process
        close(sv[0]);  // Close the parent's socket

        // Receive message from the parent
        memset(buffer, 0, BUFFER_SIZE);
        if (read(sv[1], buffer, BUFFER_SIZE) < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("Child received: %s\n", buffer);

        // Send message to the parent
        const char *child_msg = "Hello from child!";
        if (write(sv[1], child_msg, strlen(child_msg)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(sv[1]);  // Close the child's socket
    } else {  // Parent process
        close(sv[1]);  // Close the child's socket

        // Send message to the child
        const char *parent_msg = "Hello from parent!";
        if (write(sv[0], parent_msg, strlen(parent_msg)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // Receive message from the child
        memset(buffer, 0, BUFFER_SIZE);
        if (read(sv[0], buffer, BUFFER_SIZE) < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("Parent received: %s\n", buffer);

        close(sv[0]);  // Close the parent's socket
    }

    return 0;
}

