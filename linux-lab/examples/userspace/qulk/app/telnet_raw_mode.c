#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096
#define TELNET_PORT 23

// Telnet commands
#define IAC  255 // Interpret As Command
#define DONT 254
#define DO   253
#define WONT 252
#define WILL 251

// Global variables
static int sockfd = -1;
static struct termios orig_termios;
static int is_raw_mode = 0;

// Function prototypes
void cleanup(void);
void handle_signal(int sig);
int connect_to_host(const char *hostname, int port);
void set_raw_mode(void);
void restore_terminal(void);
void handle_telnet_command(unsigned char *buffer, int *len);
void telnet_negotiate(unsigned char *buffer, int len);

// Signal handler
void handle_signal(int sig) {
    cleanup();
    exit(sig);
}

// Cleanup function
void cleanup(void) {
    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }
    if (is_raw_mode) {
        restore_terminal();
    }
}

// Set terminal to raw mode
void set_raw_mode(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        return;
    }

    raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return;
    }

    is_raw_mode = 1;
}

// Restore terminal to original state
void restore_terminal(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        perror("tcsetattr");
    }
    is_raw_mode = 0;
}

// Connect to remote host
int connect_to_host(const char *hostname, int port) {
    struct addrinfo hints, *res, *res0;
    int error;
    char port_str[6];
    
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    error = getaddrinfo(hostname, port_str, &hints, &res0);
    if (error) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }
    
    for (res = res0; res; res = res->ai_next) {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) {
            continue;
        }
        
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
            close(sockfd);
            sockfd = -1;
            continue;
        }
        
        break;
    }
    
    freeaddrinfo(res0);
    
    if (sockfd < 0) {
        fprintf(stderr, "Could not connect to %s:%d\n", hostname, port);
        return -1;
    }
    
    return sockfd;
}

// Handle telnet protocol negotiation
void telnet_negotiate(unsigned char *buffer, int len) {
    unsigned char response[3] = {IAC};
    
    for (int i = 0; i < len; i++) {
        if (buffer[i] == IAC && i + 1 < len) {
            i++;
            switch (buffer[i]) {
                case WILL:
                case DO:
                    response[1] = DONT;
                    break;
                case WONT:
                case DONT:
                    response[1] = DONT;
                    break;
                default:
                    continue;
            }
            
            if (i + 1 < len) {
                response[2] = buffer[i + 1];
                write(sockfd, response, 3);
                i++;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hostname> [port]\n", argv[0]);
        return 1;
    }

    const char *hostname = argv[1];
    int port = (argc > 2) ? atoi(argv[2]) : TELNET_PORT;

    // Set up signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Connect to remote host
    if (connect_to_host(hostname, port) < 0) {
        return 1;
    }

    printf("Connected to %s:%d\n", hostname, port);

    // Set terminal to raw mode
    set_raw_mode();

    // Set socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    fd_set readfds;
    unsigned char buffer[BUFFER_SIZE];
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        if (select(sockfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // Handle keyboard input
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            int n = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (n <= 0) break;
            write(sockfd, buffer, n);
        }

        // Handle socket input
        if (FD_ISSET(sockfd, &readfds)) {
            int n = read(sockfd, buffer, sizeof(buffer));
            if (n <= 0) break;
            telnet_negotiate(buffer, n);
            write(STDOUT_FILENO, buffer, n);
        }
    }

    cleanup();
    printf("\nConnection closed.\n");
    return 0;
} 