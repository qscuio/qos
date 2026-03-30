#include <stdio.h>
#include <unistd.h>
#include "gpt/wrr_thread_pool.h"

void sample_task(void *arg) {
    int task_id = *((int*)arg);
    printf("Running task %d\n", task_id);
    sleep(1);
}

int main() {
    ThreadPool pool;
    int task_ids[3] = {1, 2, 3};
    thread_pool_init(&pool, 3, 1);

    thread_pool_add_task(&pool, sample_task, &task_ids[0], 1, 4); // Task 1 with weight 3
    thread_pool_add_task(&pool, sample_task, &task_ids[1], 2, 2); // Task 2 with weight 2
    thread_pool_add_task(&pool, sample_task, &task_ids[2], 3, 1); // Task 3 with weight 1

    thread_pool_start(&pool);

    sleep(16); // Let the pool run for a while

    thread_pool_shutdown(&pool);
    return 0;
}

