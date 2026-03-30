#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    const char *fifoPath = "/tmp/my_fifo";

    // Create the FIFO (named pipe)
    if (mkfifo(fifoPath, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process: Read from the FIFO
        int fd = open(fifoPath, O_RDONLY);
        if (fd == -1) {
            perror("open (child)");
            exit(EXIT_FAILURE);
        }

        char buffer[128];
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            perror("read");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // Null-terminate the string and print it
        buffer[bytesRead] = '\0';
        printf("Child received message: %s\n", buffer);

        close(fd);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process: Write to the FIFO
        int fd = open(fifoPath, O_WRONLY);
        if (fd == -1) {
            perror("open (parent)");
            exit(EXIT_FAILURE);
        }

        const char *message = "Hello from parent!";
        write(fd, message, strlen(message));

        close(fd);

        // Wait for the child process to finish
        wait(NULL);

        // Optionally, remove the FIFO file
        if (unlink(fifoPath) == -1) {
            perror("unlink");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }
}

