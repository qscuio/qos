#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include <time.h>     // for timespec

#define SERVER_PORT 23  // Standard Telnet port
#define BUFFER_SIZE 1024

// Telnet command definitions
#define IAC 255
#define DO 253
#define DONT 254
#define WILL 251
#define WONT 252
#define ECHO 1  // Telnet option for Echo (RFC 857)

#define MAX_EVENTS 64
#define MAX_IMMEDIATE_EVENTS 10

typedef struct Event {
    int fd;
    void (*callback)(int, uint32_t, void*);
    void* user_data;
} Event;

typedef struct ImmediateEvent {
    void (*callback)(void*);
    void* user_data;
} ImmediateEvent;

typedef struct EventLoop {
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
    ImmediateEvent immediate_events[MAX_IMMEDIATE_EVENTS];
    int immediate_count;
} EventLoop;

// Initialize the event loop
EventLoop* event_loop_create() {
    EventLoop* loop = (EventLoop*)malloc(sizeof(EventLoop));
    if (!loop) {
        perror("Failed to allocate memory for event loop");
        return NULL;
    }
    
    loop->epoll_fd = epoll_create1(0);
    if (loop->epoll_fd == -1) {
        perror("Failed to create epoll instance");
        free(loop);
        return NULL;
    }
    
    loop->immediate_count = 0;
    return loop;
}

// Set a file descriptor to non-blocking mode
static int make_fd_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL) failed");
        return -1;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL) failed");
        return -1;
    }
    
    return 0;
}

// Register a file descriptor with the event loop
int event_loop_register(EventLoop* loop, int fd, uint32_t events, void (*callback)(int, uint32_t, void*), void* user_data) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = malloc(sizeof(Event));
    if (!ev.data.ptr) {
        perror("Failed to allocate memory for event data");
        return -1;
    }

    Event* event = (Event*)ev.data.ptr;
    event->fd = fd;
    event->callback = callback;
    event->user_data = user_data;

    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("Failed to add file descriptor to epoll");
        free(ev.data.ptr);
        return -1;
    }

    return make_fd_non_blocking(fd);
}

// Unregister a file descriptor from the event loop
int event_loop_unregister(EventLoop* loop, int fd) {
    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("Failed to remove file descriptor from epoll");
        return -1;
    }
    return 0;
}

// Create a timer and register it as an event
int event_loop_add_timer(EventLoop* loop, int interval_sec, void (*callback)(int, uint32_t, void*), void* user_data) {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd == -1) {
        perror("Failed to create timer fd");
        return -1;
    }

    struct itimerspec new_value;
    new_value.it_value.tv_sec = interval_sec;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = interval_sec;
    new_value.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &new_value, NULL) == -1) {
        perror("Failed to set timer");
        close(timer_fd);
        return -1;
    }

    return event_loop_register(loop, timer_fd, EPOLLIN, callback, user_data);
}

// Add an immediate event (to be executed on the next loop iteration)
int event_loop_add_immediate(EventLoop* loop, void (*callback)(void*), void* user_data) {
    if (loop->immediate_count >= MAX_IMMEDIATE_EVENTS) {
        fprintf(stderr, "Too many immediate events\n");
        return -1;
    }

    loop->immediate_events[loop->immediate_count].callback = callback;
    loop->immediate_events[loop->immediate_count].user_data = user_data;
    loop->immediate_count++;

    return 0;
}

// Start the event loop
void event_loop_run(EventLoop* loop) {
    while (1) {
        // First, execute all immediate events
        for (int i = 0; i < loop->immediate_count; i++) {
            loop->immediate_events[i].callback(loop->immediate_events[i].user_data);
        }
        loop->immediate_count = 0;

        // Wait for epoll events
        int n = epoll_wait(loop->epoll_fd, loop->events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) {
                continue; // Restart if interrupted by a signal
            }
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < n; i++) {
            Event* event = (Event*)loop->events[i].data.ptr;
            event->callback(event->fd, loop->events[i].events, event->user_data);
        }
    }
}

// Cleanup the event loop
void event_loop_destroy(EventLoop* loop) {
    close(loop->epoll_fd);
    free(loop);
}

// Example callback for timer
void timer_event_callback(int fd, uint32_t events, void* user_data) {
    uint64_t expirations;
    read(fd, &expirations, sizeof(expirations));
    printf("Timer expired %lu times\n", expirations);
}

// Example callback for immediate event
void immediate_event_callback(void* user_data) {
    printf("Immediate event triggered: %s\n", (char*)user_data);
}

// Example callback for stdin event
void stdin_event_callback(int fd, uint32_t events, void* user_data) {
    if (events & EPOLLIN) {
        char buffer[512];
        int n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            printf("Read %d bytes: %.*s\n", n, n, buffer);
        } else if (n == 0) {
            printf("EOF on fd %d\n", fd);
            event_loop_unregister((EventLoop*)user_data, fd);
        } else {
            perror("Read failed");
        }
    }
}

// Logging helper function
void log_telnet_command(const char* message, unsigned char command, unsigned char option) {
    printf("%s: Command: %d, Option: %d\n", message, command, option);
}

// Handle incoming Telnet commands and options
void process_telnet_command(int server_fd, unsigned char command, unsigned char option) {
    unsigned char response[3] = {IAC, 0, option};
    
    if (command == WILL) {
        log_telnet_command("Received WILL", command, option);
        if (option == ECHO) {
            // Accept ECHO, send DO
            response[1] = DO;
        } else {
            // Reject other options, send DONT
            response[1] = DONT;
        }
    } else if (command == DO) {
        log_telnet_command("Received DO", command, option);
        if (option == ECHO) {
            // Accept ECHO, send WILL
            response[1] = WILL;
        } else {
            // Reject other options, send WONT
            response[1] = WONT;
        }
    } else if (command == WONT || command == DONT) {
        // Log and ignore WONT/DONT commands
        log_telnet_command("Received WONT/DONT", command, option);
        return;
    }
    
    // Send response back to server
    write(server_fd, response, 3);
}

// Callback to handle server data
void handle_telnet_server_event(int server_fd, uint32_t events, void* user_data) {
    if (events & EPOLLIN) {
        char buffer[BUFFER_SIZE];
        int n = read(server_fd, buffer, sizeof(buffer));
        if (n > 0) {
            // Process the received data and Telnet commands
            for (int i = 0; i < n; i++) {
                if ((unsigned char)buffer[i] == IAC) {
                    if (i + 2 < n) {
                        unsigned char command = buffer[i+1];
                        unsigned char option = buffer[i+2];
                        i += 2; // Skip the IAC, command, and option
                        process_telnet_command(server_fd, command, option);
                    }
                } else {
                    // Print normal characters (non-command data)
                    putchar(buffer[i]);
                    fflush(stdout);  // Ensure output is printed immediately
                }
            }
        } else if (n == 0) {
            printf("Server closed connection\n");
            close(server_fd);
            event_loop_unregister((EventLoop*)user_data, server_fd);
        } else {
            perror("Read failed");
        }
    }
}

// Callback to handle stdin input (user typing in the terminal)
void handle_stdin_event(int fd, uint32_t events, void* user_data) {
    if (events & EPOLLIN) {
        char buffer[BUFFER_SIZE];
        int n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            int server_fd = *(int*)user_data;
            write(server_fd, buffer, n);  // Send input to server
        }
    }
}

// Connect to a Telnet server
int connect_to_telnet_server(EventLoop* loop, const char* ip, int port) {
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Failed to create socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(server_fd);
        return -1;
    }

    if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(server_fd);
        return -1;
    }

    printf("Connected to Telnet server at %s:%d\n", ip, port);

    // Register the server socket with the event loop
    event_loop_register(loop, server_fd, EPOLLIN, handle_telnet_server_event, loop);

    return server_fd;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* server_ip = argv[1];

    EventLoop* loop = event_loop_create();
    if (!loop) {
        return EXIT_FAILURE;
    }

    int server_fd = connect_to_telnet_server(loop, server_ip, SERVER_PORT);
    if (server_fd == -1) {
        event_loop_destroy(loop);
        return EXIT_FAILURE;
    }

    // Register stdin to read user input
    event_loop_register(loop, STDIN_FILENO, EPOLLIN, handle_stdin_event, &server_fd);

    // Start event loop
    event_loop_run(loop);
    event_loop_destroy(loop);

    return EXIT_SUCCESS;
}

