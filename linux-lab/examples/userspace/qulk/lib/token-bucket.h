#ifndef __HAVE_TOKEN_BUCKET_H__
#define __HAVE_TOKEN_BUCKET_H__

#include <limits.h>
#include <stdbool.h>

struct token_bucket {
    /* Configuration settings. */
    unsigned int rate;          /* Tokens added per millisecond. */
    unsigned int burst;         /* Max cumulative tokens credit. */

    /* Current status. */
    unsigned int tokens;        /* Current number of tokens. */
    long long int last_fill;    /* Last time tokens added. */
};

#define TOKEN_BUCKET_INIT(RATE, BURST) { RATE, BURST, 0, LLONG_MIN }

void token_bucket_init(struct token_bucket *,
                       unsigned int rate, unsigned int burst);
void token_bucket_set(struct token_bucket *,
                       unsigned int rate, unsigned int burst);
bool token_bucket_withdraw(struct token_bucket *, unsigned int n);
void token_bucket_wait_at(struct token_bucket *, unsigned int n,
                          const char *where);
#define token_bucket_wait(bucket, n)                    \
    token_bucket_wait_at(bucket, n, OVS_SOURCE_LOCATOR)

#endif /* __HAVE_TOKEN_BUCKET_H__ */
