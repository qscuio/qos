#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

/*

To support both CIR (Committed Information Rate) and PIR (Peak Information Rate) in a rate limiter, we need to manage two different rates and two different token buckets:

CIR (Committed Information Rate): This defines the guaranteed or sustained rate of traffic that is allowed.
PIR (Peak Information Rate): This defines the maximum burst traffic that can be allowed, exceeding the CIR but within a peak burst limit.

Plan:

Use two token buckets:

* One for the CIR to handle sustained traffic.
* Another for the PIR to handle burst traffic, which fills up faster but has a higher maximum capacity.

* When a request comes:

* If PIR tokens are available, a request should be allowed (to handle burst traffic), regardless of whether CIR tokens are available.
* If CIR tokens are available and PIR is not, we should allow a request but limit it to the sustained rate.
* If neither CIR nor PIR tokens are available, the request should be rate-limited.

CIR: Represents the guaranteed, sustained rate of requests. This is the number of requests allowed per second under normal traffic conditions, without any bursts.
	If the rate limiter only used CIR, then the long-term average rate would be exactly equal to the CIR.

PIR: Allows handling of bursts by allowing a higher rate for a short period, until the PIR bucket is drained. After that, the system falls back to the CIR rate.
	PIR allows momentary traffic spikes, but its effect on the long-term average depends on how frequently burst requests occur.

Factors Affecting the Average Rate:
	CIR Rate (cir_rate): The number of requests per second allowed under normal conditions.
	PIR Rate (pir_rate): The number of requests per second allowed during burst periods.
	Burst Size (pir_burst_size): The maximum burst capacity. This determines how many extra requests can be allowed during bursts.
	Time Between Bursts: The amount of time the system spends under the PIR influence versus under the CIR-only regime.

Average Rate Calculation:

	Over long periods of time, the average rate tends to approach the CIR rate, because bursts are short-lived and the system will mostly operate under CIR.
	The PIR allows for short-term bursts, but after the PIR tokens are exhausted, the system will drop back to the CIR rate.

*/
typedef struct {
    double tokens;           // Current number of tokens
    double rate;             // Tokens added per second
    double burst_size;       // Max tokens (bucket size)
    struct timespec last_ts; // Last timestamp when tokens were added
} TokenBucket;

typedef struct {
    TokenBucket cir;         // CIR token bucket (Committed Rate)
    TokenBucket pir;         // PIR token bucket (Peak Rate)
} Ratelimiter;

// Initialize a token bucket with rate and burst size
int tokenbucket_init(TokenBucket *tb, double rate, double burst_size) {
    if (rate <= 0 || burst_size <= 0) {
        return -1;  // Invalid rate or burst size
    }
    tb->rate = rate;
    tb->burst_size = burst_size;
    tb->tokens = burst_size;  // Start with a full bucket (maximum burst)
    clock_gettime(CLOCK_MONOTONIC, &tb->last_ts);  // Record the current time
    return 0;
}

// Get the current time in seconds (with nanosecond precision)
double get_time_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

// Update the tokens in a bucket based on elapsed time
void tokenbucket_add_tokens(TokenBucket *tb) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);  // Get current time
    double elapsed = get_time_diff(&tb->last_ts, &now);  // Time since last token update
    tb->tokens += elapsed * tb->rate;  // Add tokens based on elapsed time
    if (tb->tokens > tb->burst_size) {
        tb->tokens = tb->burst_size;  // Cap tokens at burst size
    }
    tb->last_ts = now;  // Update the last timestamp
}

// Initialize the rate limiter with CIR and PIR configurations
int ratelimiter_init(Ratelimiter *rl, double cir_rate, double cir_burst_size, 
                     double pir_rate, double pir_burst_size) {
    if (tokenbucket_init(&rl->cir, cir_rate, cir_burst_size) != 0 ||
        tokenbucket_init(&rl->pir, pir_rate, pir_burst_size) != 0) {
        return -1;
    }
    return 0;
}

int ratelimiter_allow(Ratelimiter *rl) {
    // Update tokens in both CIR and PIR buckets
    tokenbucket_add_tokens(&rl->cir);
    tokenbucket_add_tokens(&rl->pir);

    // Priority: Use CIR tokens first for committed rate
    if (rl->cir.tokens >= 1.0) {
        rl->cir.tokens -= 1.0;  // Consume one CIR token
        return 1;  // Request allowed at committed rate (CIR)
    }
    // If CIR tokens are exhausted, allow burst traffic using PIR tokens
    else if (rl->pir.tokens >= 1.0) {
        rl->pir.tokens -= 1.0;  // Consume one PIR token
        return 1;  // Request allowed at peak rate (PIR)
    }

    return 0;  // Request rate-limited
}

// Clean up resources (optional in this case, no dynamic memory allocated)
void ratelimiter_destroy(Ratelimiter *rl) {
    // Nothing to clean up in this simple example
}

int main() {
    Ratelimiter rl;
    double cir_rate = 5.0;  // 5 tokens per second (CIR)
    double cir_burst_size = 10.0;  // Max 10 requests for CIR burst
    double pir_rate = 10.0;  // 10 tokens per second (PIR for peak rate)
    double pir_burst_size = 15.0;  // Max 15 requests for PIR burst

    // Initialize the rate limiter with both CIR and PIR
    if (ratelimiter_init(&rl, cir_rate, cir_burst_size, pir_rate, pir_burst_size) != 0) {
        fprintf(stderr, "Failed to initialize rate limiter\n");
        return 1;
    }

    // Simulate a loop where we check if requests can proceed
	for (int i = 0; i < 100; ++i) {
		if (ratelimiter_allow(&rl)) {
			printf("Request %d allowed (CIR tokens: %.2f, PIR tokens: %.2f)\n", i + 1, rl.cir.tokens, rl.pir.tokens);
		} else {
			printf("Request %d rate-limited (CIR tokens: %.2f, PIR tokens: %.2f)\n", i + 1, rl.cir.tokens, rl.pir.tokens);
		}
		usleep(20000);  // Simulate 200ms between requests
	}

	// Clean up the rate limiter
    ratelimiter_destroy(&rl);
    return 0;
}

