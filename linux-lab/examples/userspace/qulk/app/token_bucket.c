#include "poll-loop.h"
#include "token-bucket.h"
#include "util.h"
#include <stdio.h>

int main(int argc, char **argv) {
    int                 i  = 0;
    struct token_bucket tb = TOKEN_BUCKET_INIT(1, 10000);

    for (i = 0; i < 1000000; i++) {
        if (token_bucket_withdraw(&tb, 2000)) {
            printf("Get token\n");
        } else {
            printf("Wait for token\n");
            token_bucket_wait(&tb, 2000);
            poll_block();
        }
    }

    return 0;
}
