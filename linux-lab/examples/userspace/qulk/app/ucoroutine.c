#include "ucoroutine.h"
#include <assert.h>
#include <stdio.h>

int hello_world(coro_t *coro) {
    printf("%s\n", "Hello");
    coro_yield(coro, 1); // Suspension point that returns the value `1`
    printf("%s\n", "World");
    return 2;
}

int hello_word(coro_t *coro) {
    printf("%s\n", "Hello");
    coro_yield(coro, 3); // Suspension point that returns the value `1`
    printf("%s\n", "Word");
    return 4;
}

int main() {
    coro_t *coro  = coro_new(hello_world);
    coro_t *coro1 = coro_new(hello_word);
    assert(coro_resume(coro) == 1);   // Verifying return value
    assert(coro_resume(coro) == 2);   // Verifying return value
    assert(coro_resume(coro) == -1);  // Invalid call
    assert(coro_resume(coro1) == 3);  // Verifying return value
    assert(coro_resume(coro1) == 4);  // Verifying return value
    assert(coro_resume(coro1) == -1); // Invalid call
    coro_free(coro);
    coro_free(coro1);
    return EXIT_SUCCESS;
}
