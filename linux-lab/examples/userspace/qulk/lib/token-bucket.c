#include "token-bucket.h"
#include "poll-loop.h"
#include "sat-math.h"
#include "timeval.h"
#include "util.h"

/* Initializes 'tb' to accumulate 'rate' tokens per millisecond, with a
 * maximum of 'burst' tokens.
 *
 * The token bucket is initially full.
 *
 * It may be more convenient to use TOKEN_BUCKET_INIT. */
void
token_bucket_init(struct token_bucket *tb,
                  unsigned int rate, unsigned int burst)
{
    tb->rate = rate;
    tb->burst = burst;
    tb->tokens = 0;
    tb->last_fill = LLONG_MIN;
}

/* Changes 'tb' to accumulate 'rate' tokens per millisecond, with a maximum of
 * 'burst' tokens.
 *
 * 'tb' must already have been initialized with TOKEN_BUCKET_INIT or
 * token_bucket_init(). */
void
token_bucket_set(struct token_bucket *tb,
                 unsigned int rate, unsigned int burst)
{
    tb->rate = rate;
    tb->burst = burst;
    if (burst < tb->tokens) {
        tb->tokens = burst;
    }
}

/* Attempts to remove 'n' tokens from 'tb'.  Returns true if successful, false
 * if 'tb' contained fewer than 'n' tokens (and thus 'n' tokens could not be
 * removed) . */
bool
token_bucket_withdraw(struct token_bucket *tb, unsigned int n)
{
    if (tb->tokens < n) {
        long long int now = time_msec();
        if (now > tb->last_fill) {
            unsigned long long int elapsed_ull
                = (unsigned long long int) now - tb->last_fill;
            unsigned int elapsed = MIN(UINT_MAX, elapsed_ull);
            unsigned int add = sat_mul(tb->rate, elapsed);
            unsigned int tokens = sat_add(tb->tokens, add);
            tb->tokens = MIN(tokens, tb->burst);
            tb->last_fill = now;
        }

        if (tb->tokens < n) {
            return false;
        }
    }

    tb->tokens -= n;
    return true;
}

/* Causes the poll loop to wake up when at least 'n' tokens will be available
 * for withdrawal from 'tb'. */
void
token_bucket_wait_at(struct token_bucket *tb, unsigned int n,
                     const char *where)
{
    if (tb->tokens >= n) {
        poll_immediate_wake_at(where);
    } else {
        unsigned int need = n - tb->tokens;
        poll_timer_wait_until_at(tb->last_fill + need / tb->rate + 1, where);
    }
}
