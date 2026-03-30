#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

typedef struct {
    unsigned int capacity;          // Maximum tokens the bucket can hold
    unsigned int tokens;            // Current number of tokens in the bucket
    struct timeval last_update;     // Last time the bucket was updated
    double fill_rate;               // Tokens filled per second
} TokenBucket;

// Initializes a token bucket with the given capacity and fill rate
void token_bucket_init(TokenBucket *bucket, unsigned int capacity, double fill_rate) {
    bucket->capacity = capacity;
    bucket->tokens = capacity;
    gettimeofday(&bucket->last_update, NULL);
    bucket->fill_rate = fill_rate;
}

// Updates the number of tokens in the bucket based on the time elapsed since the last update
void token_bucket_update(TokenBucket *bucket) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    double elapsed_time = (current_time.tv_sec - bucket->last_update.tv_sec) +
                          (current_time.tv_usec - bucket->last_update.tv_usec) / 1000000.0;

    unsigned int tokens_to_add = (unsigned int)(elapsed_time * bucket->fill_rate);
    bucket->tokens = (bucket->tokens + tokens_to_add <= bucket->capacity) ?
                     bucket->tokens + tokens_to_add : bucket->capacity;

    bucket->last_update = current_time;
}

// Attempts to consume the specified number of tokens from the bucket
// Returns true if successful, false otherwise
bool token_bucket_consume(TokenBucket *bucket, unsigned int tokens) {
    token_bucket_update(bucket);

    if (tokens <= bucket->tokens) {
        bucket->tokens -= tokens;
        return true;
    }

    return false;
}

int main() {
    TokenBucket bucket;
    unsigned int tokens_to_consume = 11;

    token_bucket_init(&bucket, 10, 2.0);  // Create a token bucket with capacity 10 and fill rate 2 tokens/sec

    if (token_bucket_consume(&bucket, tokens_to_consume)) {
        printf("Tokens consumed: %u\n", tokens_to_consume);
    } else {
        printf("Not enough tokens available\n");
    }

    return 0;
}

