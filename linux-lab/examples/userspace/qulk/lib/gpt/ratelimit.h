#ifndef RATELIMIT_H
#define RATELIMIT_H

#include <stdint.h>
#include <time.h>

typedef struct {
    uint64_t committed_burst_size; // CBS
    uint64_t excess_burst_size;    // EBS
    uint64_t committed_rate;       // CIR (in tokens per second)
    uint64_t excess_rate;          // EIR (in tokens per second)
    uint64_t committed_bucket;     // current size of committed bucket
    uint64_t excess_bucket;        // current size of excess bucket
    struct timespec last_time;     // last time the rate limiter was checked
} RateLimiter;

// Initializes the rate limiter
void ratelimit_init(RateLimiter *rl, uint64_t cbs, uint64_t ebs, uint64_t cir, uint64_t eir);

// Checks if a task consuming `tokens` can be executed, returns 0 for red, 1 for yellow, 2 for green
int ratelimit_check(RateLimiter *rl, uint64_t tokens);

#endif // RATELIMIT_H

