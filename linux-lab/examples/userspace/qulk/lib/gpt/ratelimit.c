#include "ratelimit.h"
#include <stdlib.h>
#include <time.h>

static uint64_t time_diff_ns(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000000000ULL + (end->tv_nsec - start->tv_nsec);
}

void ratelimit_init(RateLimiter *rl, uint64_t cbs, uint64_t ebs, uint64_t cir, uint64_t eir) {
    rl->committed_burst_size = cbs;
    rl->excess_burst_size = ebs;
    rl->committed_rate = cir;
    rl->excess_rate = eir;
    rl->committed_bucket = cbs;
    rl->excess_bucket = ebs;
    clock_gettime(CLOCK_MONOTONIC, &rl->last_time);
}

int ratelimit_check(RateLimiter *rl, uint64_t tokens) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    uint64_t elapsed_ns = time_diff_ns(&rl->last_time, &now);
    rl->last_time = now;

    uint64_t add_committed_tokens = (rl->committed_rate * elapsed_ns) / 1000000000ULL;
    uint64_t add_excess_tokens = (rl->excess_rate * elapsed_ns) / 1000000000ULL;

    rl->committed_bucket = (rl->committed_bucket + add_committed_tokens > rl->committed_burst_size) ? 
                            rl->committed_burst_size : rl->committed_bucket + add_committed_tokens;
    rl->excess_bucket = (rl->excess_bucket + add_excess_tokens > rl->excess_burst_size) ? 
                        rl->excess_burst_size : rl->excess_bucket + add_excess_tokens;

    if (tokens <= rl->committed_bucket) {
        rl->committed_bucket -= tokens;
        return 2; // green
    } else if (tokens <= rl->committed_bucket + rl->excess_bucket) {
        uint64_t excess_used = tokens - rl->committed_bucket;
        rl->committed_bucket = 0;
        rl->excess_bucket -= excess_used;
        return 1; // yellow
    } else {
        return 0; // red
    }
}

