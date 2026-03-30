#include <ev.h>
#include <stdio.h>

// Callback function for the timer event
static void timeout_cb(EV_P_ ev_timer *w, int revents) {
    puts("Timeout!");
    // Stop the event loop
    ev_break(EV_A_ EVBREAK_ALL);
}

int main() {
    // Create the default event loop
    struct ev_loop *loop = EV_DEFAULT;

    // Create and initialize a timer watcher
    ev_timer timeout_watcher;
    ev_timer_init(&timeout_watcher, timeout_cb, 5., 0.);

    // Start the timer watcher
    ev_timer_start(loop, &timeout_watcher);

    // Start the event loop
    ev_run(loop, 0);

    return 0;
}

