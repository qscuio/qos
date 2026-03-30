#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "future.h"

void *func(void *arg) {
    promise *p = (promise *) arg;
    printf("\tstarted thread\n");
    sleep(3);
    printf("\tthread will set promise\n");
    promise_set(p, 42);
    printf("\tstopping thread\n");
    return NULL;
}

int main() {
    printf("main thread\n");
    future  *f = future_create(func);
    promise *p = promise_create();
    future_start(f, p);

    printf("got result from future: %d\n", promise_get(p));

    promise_close(p);
    future_close(f);
    pthread_exit(NULL);
}
