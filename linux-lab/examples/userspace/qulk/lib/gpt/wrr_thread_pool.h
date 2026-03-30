#ifndef WRR_THREAD_POOL_H
#define WRR_THREAD_POOL_H

#include <pthread.h>

typedef struct {
    int task_id;
    int weight;
    void (*function)(void*);
    void *arg;
    int remaining_quanta;
} Task;

typedef struct {
    Task *tasks;
    int num_tasks;
    int current_task_index;
    int quanta_per_round;
    pthread_mutex_t lock;
} Scheduler;

typedef struct {
    pthread_t *threads;
    int num_threads;
    Scheduler scheduler;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_cond;
    int shutdown;
} ThreadPool;

// Initialize the thread pool with a given number of threads and quanta per round for WRR
int thread_pool_init(ThreadPool *pool, int num_threads, int quanta_per_round);

// Add a task to the thread pool with a specific weight
int thread_pool_add_task(ThreadPool *pool, void (*function)(void*), void *arg, int task_id, int weight);

// Start the thread pool
void thread_pool_start(ThreadPool *pool);

// Shutdown the thread pool
void thread_pool_shutdown(ThreadPool *pool);

#endif // WRR_THREAD_POOL_H

