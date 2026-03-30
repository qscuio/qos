#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/timerfd.h>

#include <sys/epoll.h>
#include <time.h>

// Event types
#define EVENT_READ 0x01
#define EVENT_WRITE 0x02
#define EVENT_CONNECT 0x04
#define EVENT_TIMER 0x08

typedef struct EventLoop EventLoop;
typedef struct Event Event;

// Event handler callback function type
typedef void (*event_handler_t)(int fd, uint32_t events, void* user_data);

// Function to create a new event loop
EventLoop* event_loop_create();

// Function to register an event (read/write/connect)
int event_loop_register(EventLoop* loop, int fd, uint32_t events, event_handler_t handler, void* user_data);

// Function to unregister an event
int event_loop_unregister(EventLoop* loop, int fd);

// Function to add a timer event (timer expiration in seconds)
int event_loop_add_timer(EventLoop* loop, int interval_sec, event_handler_t handler, void* user_data);

// Function to add an immediate event (executes without waiting)
int event_loop_add_immediate(EventLoop* loop, event_handler_t handler, void* user_data);

// Function to run the event loop
void event_loop_run(EventLoop* loop);

// Function to destroy the event loop
void event_loop_destroy(EventLoop* loop);

#define MAX_EVENTS 64
#define MAX_IMMEDIATE_EVENTS 10


struct Event
{
    event_handler_t handler;
    void* user_data;
	int fd;
};

typedef struct ImmediateEvent {
    event_handler_t handler;
    void* user_data;
} ImmediateEvent;

struct EventLoop {
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
    ImmediateEvent immediate_events[MAX_IMMEDIATE_EVENTS];
    int immediate_count;
};

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

// Helper function to set non-blocking mode for file descriptors
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

int event_loop_register(EventLoop* loop, int fd, uint32_t events, event_handler_t handler, void* user_data) {
    struct epoll_event ev;
    ev.events = 0;

    if (events & EVENT_READ) ev.events |= EPOLLIN;
    if (events & EVENT_WRITE) ev.events |= EPOLLOUT;
    if (events & EVENT_CONNECT) ev.events |= EPOLLRDHUP | EPOLLOUT;

    ev.data.ptr = malloc(sizeof(Event));
    if (!ev.data.ptr) {
        perror("Failed to allocate memory for event");
        return -1;
    }

    Event* event = (Event*)ev.data.ptr;
    event->fd = fd;
    event->handler = handler;
    event->user_data = user_data;

    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("Failed to add event to epoll");
        free(ev.data.ptr);
        return -1;
    }

    return make_fd_non_blocking(fd);
}

int event_loop_unregister(EventLoop* loop, int fd) {
    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("Failed to remove event from epoll");
        return -1;
    }
    return 0;
}

// Add a timer event (timer expiration in seconds)
int event_loop_add_timer(EventLoop* loop, int interval_sec, event_handler_t handler, void* user_data) {
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

    return event_loop_register(loop, timer_fd, EVENT_READ, handler, user_data);
}

// Add an immediate event
int event_loop_add_immediate(EventLoop* loop, event_handler_t handler, void* user_data) {
    if (loop->immediate_count >= MAX_IMMEDIATE_EVENTS) {
        fprintf(stderr, "Too many immediate events\n");
        return -1;
    }

    loop->immediate_events[loop->immediate_count].handler = handler;
    loop->immediate_events[loop->immediate_count].user_data = user_data;
    loop->immediate_count++;

    return 0;
}

// Main event loop
void event_loop_run(EventLoop* loop) {
    while (1) {
        // Execute immediate events
        for (int i = 0; i < loop->immediate_count; i++) {
            ImmediateEvent* imm_event = &loop->immediate_events[i];
            imm_event->handler(-1, 0, imm_event->user_data);  // Immediate events have no fd or events
        }
        loop->immediate_count = 0;

        // Wait for epoll events
        int n = epoll_wait(loop->epoll_fd, loop->events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;  // Restart if interrupted by a signal
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < n; i++) {
            Event* event = (Event*)loop->events[i].data.ptr;
            event->handler(event->fd, loop->events[i].events, event->user_data);
        }
    }
}

void event_loop_destroy(EventLoop* loop) {
    close(loop->epoll_fd);
    free(loop);
}





#define SERVER_PORT 23  // Standard Telnet port
#define BUFFER_SIZE 1024

// Telnet command definitions
#define IAC 255
#define DO 253
#define DONT 254
#define WILL 251
#define WONT 252
#define ECHO 1  // Telnet option for Echo (RFC 857)
				
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

// Handle incoming data from the server
void handle_server_event(int fd, uint32_t events, void* user_data) {
    if (events & EPOLLIN) {
        char buffer[BUFFER_SIZE];
        int n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                if ((unsigned char)buffer[i] == IAC) {
                    if (i + 2 < n) {
                        unsigned char command = buffer[i+1];
                        unsigned char option = buffer[i+2];
                        i += 2; // Skip the IAC, command, and option
                        process_telnet_command(fd, command, option);
                    }
                } else {
                    // Print normal characters (non-command data)
                    putchar(buffer[i]);
                    fflush(stdout);  // Ensure output is printed immediately
                }
			}
        } else if (n == 0) {
            printf("Server closed connection.\n");
            event_loop_unregister((EventLoop*)user_data, fd);
            close(fd);
        } else {
            perror("Read failed");
        }
    }
}

// Handle user input from stdin
void handle_stdin_event(int fd, uint32_t events, void* user_data) {
    char buffer[BUFFER_SIZE];
    int n = read(fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        int server_fd = *(int*)user_data;
        write(server_fd, buffer, n);
    }
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* server_ip = argv[1];
    int server_fd;
    struct sockaddr_in server_addr;

    // Set up connection to the Telnet server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Set up event loop
    EventLoop* loop = event_loop_create();
    if (!loop) {
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Register the server socket for reading
    event_loop_register(loop, server_fd, EVENT_READ, handle_server_event, loop);

    // Register stdin for reading user input
    event_loop_register(loop, STDIN_FILENO, EVENT_READ, handle_stdin_event, &server_fd);

    printf("Connected to Telnet server at %s\n", server_ip);

    // Run the event loop
    event_loop_run(loop);
    event_loop_destroy(loop);
    
    close(server_fd);
    return EXIT_SUCCESS;
}
