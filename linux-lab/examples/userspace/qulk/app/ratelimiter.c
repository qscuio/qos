#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>

typedef struct {
    double tokens;           // Current number of tokens
    double rate;             // Tokens added per second
    double burst_size;       // Max tokens (bucket size)
    struct timespec last_ts; // Last timestamp when tokens were added
} Ratelimiter;

// Initialize the rate limiter with the given rate (tokens/sec) and burst size
int ratelimiter_init(Ratelimiter *rl, double rate, double burst_size) {
    if (rate <= 0 || burst_size <= 0) {
        return -1;  // Invalid rate or burst size
    }
    rl->rate = rate;
    rl->burst_size = burst_size;
    rl->tokens = burst_size;  // Start with a full bucket (maximum burst)
    clock_gettime(CLOCK_MONOTONIC, &rl->last_ts);  // Record the current time
    return 0;
}

// Get the current time in seconds (with nanosecond precision)
double get_time_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

// Update the tokens in the bucket based on elapsed time
void ratelimiter_add_tokens(Ratelimiter *rl) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);  // Get current time
    double elapsed = get_time_diff(&rl->last_ts, &now);  // Time since last token update
    rl->tokens += elapsed * rl->rate;  // Add tokens based on elapsed time
    if (rl->tokens > rl->burst_size) {
        rl->tokens = rl->burst_size;  // Cap tokens at burst size
    }
    rl->last_ts = now;  // Update the last timestamp
}

// Check if a request is allowed, consuming one token if allowed
int ratelimiter_allow(Ratelimiter *rl) {
    ratelimiter_add_tokens(rl);  // Update tokens
    if (rl->tokens >= 1.0) {
        rl->tokens -= 1.0;  // Consume a token
        return 1;  // Request allowed
    }
    return 0;  // Request not allowed (rate-limited)
}

// Clean up resources (optional in this case, no dynamic memory allocated)
void ratelimiter_destroy(Ratelimiter *rl) {
    // Nothing to clean up in this simple example
}

int main() {
    Ratelimiter rl;
    double rate = 5.0;  // 5 tokens per second
    double burst_size = 10.0;  // Max burst of 10 requests

    // Initialize the rate limiter
    if (ratelimiter_init(&rl, rate, burst_size) != 0) {
        fprintf(stderr, "Failed to initialize rate limiter\n");
        return 1;
    }

    // Simulate a loop where we check if requests can proceed
    for (int i = 0; i < 20; ++i) {
        if (ratelimiter_allow(&rl)) {
            printf("Request %d allowed\n", i + 1);
        } else {
            printf("Request %d rate-limited\n", i + 1);
        }
        usleep(40000);
    }

    // Clean up the rate limiter
    ratelimiter_destroy(&rl);
    return 0;
}

