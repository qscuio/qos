#include <stdio.h>
#include "gpt/ratelimit.h"

void perform_task(int result) {
    if (result == 2) {
        printf("Task executed: green\n");
    } else if (result == 1) {
        printf("Task executed: yellow\n");
    } else {
        printf("Task blocked: red\n");
    }
}

int main() {
    RateLimiter rl;
    ratelimit_init(&rl, 1000, 2000, 500, 1000); // Initialize with example values

    int result = ratelimit_check(&rl, 300);
    perform_task(result);

    result = ratelimit_check(&rl, 700);
    perform_task(result);

    result = ratelimit_check(&rl, 2000);
    perform_task(result);

    return 0;
}

