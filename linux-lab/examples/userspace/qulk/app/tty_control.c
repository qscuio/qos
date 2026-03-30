#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int main() {
    struct termios tty;

    // Get current terminal attributes
    if (tcgetattr(STDIN_FILENO, &tty) != 0) {
        perror("tcgetattr");
        return 1;
    }

    // Set terminal to raw mode
    tty.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tty.c_cc[VMIN] = 1;              // Minimum number of characters to read
    tty.c_cc[VTIME] = 0;             // No timeout

    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return 1;
    }

    printf("Terminal set to raw mode. Press 'q' to exit.\n");

    char c;
    while (1) {
        read(STDIN_FILENO, &c, 1);
        if (c == 'q')
            break;
        printf("You pressed: %c\n", c);
    }

    // Restore terminal attributes
    tty.c_lflag |= (ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return 1;
    }

    printf("Terminal restored to normal mode.\n");
    return 0;
}

