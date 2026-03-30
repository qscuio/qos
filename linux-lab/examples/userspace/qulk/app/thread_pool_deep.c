#ifndef THPOOL_H
#define THPOOL_H

typedef struct ThreadPool ThreadPool;

ThreadPool* thpool_create(int num_threads);
int thpool_add_task(ThreadPool* pool, void (*func)(void*), void* arg);
void thpool_resize(ThreadPool* pool, int new_num);
void thpool_destroy(ThreadPool* pool);

#endif // THPOOL_H

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
//#include "thpool.h"

typedef struct Task {
    void (*func)(void*);
    void* arg;
    struct Task* next;
} Task;

struct ThreadPool {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_cond_t cond_no_threads;
    pthread_cond_t cond_all_tasks_done;  // NEW: For task completion tracking
    Task* task_queue_head;
    Task* task_queue_tail;
    int tasks_pending;                   // NEW: Total tasks (queued + in-progress)
    int queue_size;
    int current_num_threads;
    int target_num_threads;
    int shutdown;
};

static void* worker_thread(void* arg);

ThreadPool* thpool_create(int num_threads) {
    ThreadPool* pool = malloc(sizeof(ThreadPool));
    if (!pool) return NULL;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&pool->mutex, NULL) != 0 ||
        pthread_cond_init(&pool->cond, NULL) != 0 ||
        pthread_cond_init(&pool->cond_no_threads, NULL) != 0 ||
        pthread_cond_init(&pool->cond_all_tasks_done, NULL) != 0) {  // NEW
        free(pool);
        return NULL;
    }

    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;
    pool->queue_size = 0;
    pool->tasks_pending = 0;             // NEW
    pool->current_num_threads = 0;
    pool->target_num_threads = num_threads;
    pool->shutdown = 0;

    thpool_resize(pool, num_threads);
    return pool;
}

int thpool_add_task(ThreadPool* pool, void (*func)(void*), void* arg) {
    if (!pool || !func) return -1;

    Task* task = malloc(sizeof(Task));
    if (!task) return -1;

    task->func = func;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->mutex);
    if (pool->shutdown) {
        free(task);
        pthread_mutex_unlock(&pool->mutex);
        return -1;
    }

    // Add task to queue
    if (pool->task_queue_tail) {
        pool->task_queue_tail->next = task;
    } else {
        pool->task_queue_head = task;
    }
    pool->task_queue_tail = task;
    pool->queue_size++;
    pool->tasks_pending++;               // NEW: Track pending tasks
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}



void thpool_resize(ThreadPool* pool, int new_num) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);
    if (new_num < 0) new_num = 0;

    pool->target_num_threads = new_num;

    int to_create = new_num - pool->current_num_threads;
    if (to_create > 0) {
        for (int i = 0; i < to_create; i++) {
            pthread_t thread;
            if (pthread_create(&thread, NULL, worker_thread, pool) != 0) {
                perror("pthread_create failed");
                pool->target_num_threads--;
            } else {
                pthread_detach(thread);
            }
        }
    } else {
        pthread_cond_broadcast(&pool->cond);
    }

    pthread_mutex_unlock(&pool->mutex);
}

void thpool_wait(ThreadPool* pool) {      // NEW: Wait for all tasks to complete
    if (!pool) return;
    pthread_mutex_lock(&pool->mutex);
    while (pool->tasks_pending > 0) {
        pthread_cond_wait(&pool->cond_all_tasks_done, &pool->mutex);
    }
    pthread_mutex_unlock(&pool->mutex);
}

void thpool_destroy(ThreadPool* pool) {   // Already handles graceful shutdown
    if (!pool) return;
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pool->target_num_threads = 0;
    pthread_cond_broadcast(&pool->cond);

    // Wait for all threads to exit
    while (pool->current_num_threads > 0) {
        pthread_cond_wait(&pool->cond_no_threads, &pool->mutex);
    }

    // Cleanup tasks (if any remained)
    Task* task = pool->task_queue_head;
    while (task) {
        Task* next = task->next;
        free(task);
        task = next;
    }

    pthread_mutex_unlock(&pool->mutex);

    // Destroy synchronization primitives
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    pthread_cond_destroy(&pool->cond_no_threads);
    pthread_cond_destroy(&pool->cond_all_tasks_done);  // NEW

    free(pool);
}

static void* worker_thread(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    pthread_mutex_lock(&pool->mutex);
    pool->current_num_threads++;
    pthread_mutex_unlock(&pool->mutex);

    while (1) {
        pthread_mutex_lock(&pool->mutex);
        
        // Wait for tasks or shutdown signal
        while (pool->queue_size == 0 && !pool->shutdown && 
               pool->current_num_threads <= pool->target_num_threads) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        // Exit conditions
        if (pool->shutdown || pool->current_num_threads > pool->target_num_threads) {
            pool->current_num_threads--;
            if (pool->current_num_threads == 0) {
                pthread_cond_signal(&pool->cond_no_threads);
            }
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        // Get a task from the queue
        Task* task = pool->task_queue_head;
        if (task) {
            pool->task_queue_head = task->next;
            if (!pool->task_queue_head) {
                pool->task_queue_tail = NULL;
            }
            pool->queue_size--;
        }
        pthread_mutex_unlock(&pool->mutex);

        // Execute task
        if (task) {
            task->func(task->arg);
            free(task);
            
            // Update pending tasks and signal if all done
            pthread_mutex_lock(&pool->mutex);
            pool->tasks_pending--;
            if (pool->tasks_pending == 0) {
                pthread_cond_signal(&pool->cond_all_tasks_done);
            }
            pthread_mutex_unlock(&pool->mutex);
        }
    }
    return NULL;
}

//#include "thpool.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void example_task(void* arg) {
    int* num = (int*) arg;
    printf("Task %d processed by thread %lu\n", *num, (unsigned long)pthread_self());
    sleep(1);
    free(num);
}

int main() {
    ThreadPool* pool = thpool_create(2);
    if (!pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        int* arg = malloc(sizeof(int));
        *arg = i;
        thpool_add_task(pool, example_task, arg);
    }

	sleep(2);
    thpool_resize(pool, 4);

    for (int i = 10; i < 20; i++) {
        int* arg = malloc(sizeof(int));
        *arg = i;
        thpool_add_task(pool, example_task, arg);
    }

    //thpool_resize(pool, 2);

	thpool_wait(pool);

    thpool_destroy(pool);
    return 0;
}
