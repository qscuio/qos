#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

int main() {
    int master_fd;
    char slave_name[100];
    struct termios tty;

    // Open a PTY master
    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0) {
        perror("posix_openpt");
        return 1;
    }

    // Grant access to the slave PTY
    if (grantpt(master_fd) < 0) {
        perror("grantpt");
        close(master_fd);
        return 1;
    }

    // Unlock the slave PTY
    if (unlockpt(master_fd) < 0) {
        perror("unlockpt");
        close(master_fd);
        return 1;
    }

    // Get the name of the slave PTY
    if (ptsname_r(master_fd, slave_name, sizeof(slave_name)) != 0) {
        perror("ptsname_r");
        close(master_fd);
        return 1;
    }

    printf("Allocated PTY: %s\n", slave_name);

    // Fork a child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(master_fd);
        return 1;
    }

    if (pid == 0) {
        // Child process
        int slave_fd = open(slave_name, O_RDWR);
        if (slave_fd < 0) {
            perror("open slave");
            return 1;
        }

        // Make the slave PTY the controlling terminal
        if (setsid() < 0) {
            perror("setsid");
            return 1;
        }

        if (ioctl(slave_fd, TIOCSCTTY, NULL) < 0) {
            perror("ioctl TIOCSCTTY");
            return 1;
        }

        // Set the terminal attributes (disable echo)
        tcgetattr(slave_fd, &tty);
        tty.c_lflag &= ~ECHO; // Disable echo
        tcsetattr(slave_fd, TCSANOW, &tty);

        // Redirect standard input/output/error to the slave PTY
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(slave_fd);

        // Execute a shell
        execlp("/bin/bash", "/bin/bash", NULL);
        perror("execlp");
        return 1;
    } else {
        // Parent process
        char buf[256];
        ssize_t nread;

        // Read from the master PTY and print the output
        while ((nread = read(master_fd, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, nread);
        }

        if (nread < 0) {
            perror("read master");
        }

        close(master_fd);
        return 0;
    }
}

