#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

// Callback function for the timer event
static void timer_cb(EV_P_ ev_timer *w, int revents) {
    puts("Timer triggered!");
}

// Callback function for the I/O event
static void io_cb(EV_P_ ev_io *w, int revents) {
    if (revents & EV_READ) {
        char buffer[1024];
        ssize_t n = read(w->fd, buffer, sizeof(buffer));
        if (n > 0) {
            buffer[n] = '\0';
            printf("Read from stdin: %s", buffer);
        } else if (n == 0) {
            // EOF
            ev_io_stop(loop, w);
            puts("stdin closed");
        } else {
            perror("read");
        }
    }
}

// Callback function for the signal event
static void signal_cb(EV_P_ ev_signal *w, int revents) {
    puts("Signal received!");
    ev_break(EV_A_ EVBREAK_ALL);
}

// Callback function for the idle event
static void idle_cb(EV_P_ ev_idle *w, int revents) {
    puts("Idle event triggered!");
    ev_idle_stop(EV_A_ w);  // Stop the idle watcher after the first trigger
}

int main() {
    struct ev_loop *loop = EV_DEFAULT;

    // Timer event
    ev_timer timer_watcher;
    ev_timer_init(&timer_watcher, timer_cb, 2., 0.);
    ev_timer_start(loop, &timer_watcher);

    // I/O event
    ev_io io_watcher;
    ev_io_init(&io_watcher, io_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &io_watcher);

    // Signal event
    ev_signal signal_watcher;
    ev_signal_init(&signal_watcher, signal_cb, SIGINT);
    ev_signal_start(loop, &signal_watcher);

    // Idle event
    ev_idle idle_watcher;
    ev_idle_init(&idle_watcher, idle_cb);
    ev_idle_start(loop, &idle_watcher);

    // Start the event loop
    ev_run(loop, 0);

    return 0;
}

