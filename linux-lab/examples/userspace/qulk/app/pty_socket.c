#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <string.h>

void configure_termios(int fd) {
    struct termios tios;
    tcgetattr(fd, &tios);
    tios.c_lflag &= ~(ECHO | ICANON);  // Disable echo and canonical mode
    tcsetattr(fd, TCSANOW, &tios);
}

int main() {
    int master_fd, slave_fd;
    pid_t pid;
    char slave_name[100];
    char buffer[256];
    ssize_t nbytes;
    int socketpair_fd[2];  // For communication between parent and child

    // Create a socket pair for communication
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketpair_fd) == -1) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    // Open a new PTY master
    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0) {
        perror("posix_openpt");
        exit(EXIT_FAILURE);
    }

    // Grant access and unlock the slave
    if (grantpt(master_fd) < 0 || unlockpt(master_fd) < 0) {
        perror("grantpt/unlockpt");
        exit(EXIT_FAILURE);
    }

    // Get the name of the slave PTY
    if (ptsname_r(master_fd, slave_name, sizeof(slave_name)) != 0) {
        perror("ptsname_r");
        exit(EXIT_FAILURE);
    }

    printf("Slave PTY name: %s\n", slave_name);

    // Fork a new process
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Close the parent's socket end
        close(socketpair_fd[0]);

        // Open the slave PTY
        slave_fd = open(slave_name, O_RDWR);
        if (slave_fd < 0) {
            perror("open slave pty");
            exit(EXIT_FAILURE);
        }

        // Set the slave PTY as the controlling terminal
        if (setsid() < 0) {
            perror("setsid");
            exit(EXIT_FAILURE);
        }

        if (ioctl(slave_fd, TIOCSCTTY, NULL) < 0) {
            perror("ioctl TIOCSCTTY");
            exit(EXIT_FAILURE);
        }

        // Redirect standard input/output/error to the slave PTY
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        // Close the master and slave file descriptors
        close(master_fd);
        close(slave_fd);

        // Run a shell (or any other command)
        execlp("/bin/bash", "/bin/bash", NULL);

        // If execlp fails
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process

        // Close the child's socket end
        close(socketpair_fd[1]);

        // Configure the master PTY (e.g., disable echo)
        configure_termios(master_fd);

        // Simulate terminal I/O by communicating with the child process over the socket
        fd_set fds;
        while (1) {
            FD_ZERO(&fds);
            FD_SET(master_fd, &fds);    // Monitor PTY master
            FD_SET(socketpair_fd[0], &fds); // Monitor socket
            int max_fd = (master_fd > socketpair_fd[0]) ? master_fd : socketpair_fd[0];

            int ready = select(max_fd + 1, &fds, NULL, NULL, NULL);
            if (ready == -1) {
                perror("select");
                exit(EXIT_FAILURE);
            }

            if (FD_ISSET(master_fd, &fds)) {
                // Read data from the PTY
                nbytes = read(master_fd, buffer, sizeof(buffer) - 1);
                if (nbytes > 0) {
                    buffer[nbytes] = '\0';
                    printf("From PTY: %s", buffer);
                    // Send the data to the child process over the socket
                    write(socketpair_fd[0], buffer, nbytes);
                }
            }

            if (FD_ISSET(socketpair_fd[0], &fds)) {
                // Read data from the socket (which simulates terminal input/output)
                nbytes = read(socketpair_fd[0], buffer, sizeof(buffer) - 1);
                if (nbytes > 0) {
                    buffer[nbytes] = '\0';
                    printf("From socket: %s", buffer);
                    // Send data back to the PTY (if needed)
                    write(master_fd, buffer, nbytes);
                }
            }
        }

        // Wait for the child process to finish
        waitpid(pid, NULL, 0);

        // Close the PTY master
        close(master_fd);
    }

    return 0;
}

