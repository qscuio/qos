#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Using mmap in a Linux signal handler is not advisable due to the restrictions on what functions can be safely called from within a signal handler. Signal handlers have to be async-signal-safe because they can interrupt the normal execution of code at any time.
 *
 * The POSIX standard defines a set of functions that are safe to call from within a signal handler. These functions are guaranteed to behave correctly even when interrupted by a signal. mmap is not included in this set of functions, meaning it is not guaranteed to be safe to use within a signal handler.
 *
 * Calling mmap from a signal handler can lead to undefined behavior, which can include deadlocks, crashes, or data corruption. If you need to allocate memory in response to a signal, it's generally better to set a flag in the signal handler and then check this flag in your main program loop, where you can safely perform memory operations.
 */

volatile sig_atomic_t signal_flag = 0;

void signal_handler(int signum) {
    signal_flag = 1;
}

int main() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (signal_flag) {
            signal_flag = 0;
            // Safe to call mmap here
            // Example: mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            printf("Signal received and mmap can be called safely now.\n");
        }
        // Other code
        sleep(1);
    }

    return 0;
}

