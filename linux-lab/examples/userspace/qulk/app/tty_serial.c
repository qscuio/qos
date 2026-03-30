#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

int main() {
    int fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct termios tty;

    // Get current attributes
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        close(fd);
        return 1;
    }

    // Set Baud Rate
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    // Set 8N1 (8 data bits, No parity, 1 stop bit)
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // Disable break processing
    tty.c_lflag = 0;                            // No signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                            // No remapping, no delays
    tty.c_cc[VMIN] = 1;                         // Read at least 1 character
    tty.c_cc[VTIME] = 1;                        // Timeout after 0.1 seconds

    // Apply the configuration
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fd);
        return 1;
    }

    char send_buf[] = "Hello, Serial Port!";
    write(fd, send_buf, sizeof(send_buf));

    char recv_buf[100];
    int n = read(fd, recv_buf, sizeof(recv_buf));
    if (n > 0) {
        recv_buf[n] = '\0';
        printf("Received: %s\n", recv_buf);
    }

    close(fd);
    return 0;
}

