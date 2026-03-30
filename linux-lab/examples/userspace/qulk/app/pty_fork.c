#define _GNU_SOURCE
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <utmp.h>
#include <termios.h>

int main() {
    int master_fd;
    pid_t pid;
    struct termios tty;

    // Fork and create a PTY
    pid = forkpty(&master_fd, NULL, NULL, NULL);
    if (pid < 0) {
        perror("forkpty");
        return 1;
    }

    if (pid == 0) {
        // Child process

        // Disable echo
        tcgetattr(STDIN_FILENO, &tty);
        tty.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &tty);

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

