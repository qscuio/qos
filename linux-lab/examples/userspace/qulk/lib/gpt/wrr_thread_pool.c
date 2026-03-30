#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "wrr_thread_pool.h"

void __init_scheduler(Scheduler *scheduler, int quanta_per_round) {
    scheduler->tasks = NULL;
    scheduler->num_tasks = 0;
    scheduler->current_task_index = 0;
    scheduler->quanta_per_round = quanta_per_round;
    pthread_mutex_init(&scheduler->lock, NULL);
}

void __add_task(Scheduler *scheduler, int task_id, int weight, void (*function)(void*), void *arg) {
    pthread_mutex_lock(&scheduler->lock);
    scheduler->tasks = realloc(scheduler->tasks, (scheduler->num_tasks + 1) * sizeof(Task));
    if (scheduler->tasks == NULL) {
        perror("Failed to add task");
        exit(EXIT_FAILURE);
    }
    scheduler->tasks[scheduler->num_tasks].task_id = task_id;
    scheduler->tasks[scheduler->num_tasks].weight = weight;
    scheduler->tasks[scheduler->num_tasks].function = function;
    scheduler->tasks[scheduler->num_tasks].arg = arg;
    scheduler->tasks[scheduler->num_tasks].remaining_quanta = weight * scheduler->quanta_per_round;
    scheduler->num_tasks++;
    pthread_mutex_unlock(&scheduler->lock);
}

Task* __get_next_task(Scheduler *scheduler) {
    pthread_mutex_lock(&scheduler->lock);
    for (int i = 0; i < scheduler->num_tasks; i++) {
        if (scheduler->tasks[scheduler->current_task_index].remaining_quanta > 0) {
            scheduler->tasks[scheduler->current_task_index].remaining_quanta--;
            Task *task = &scheduler->tasks[scheduler->current_task_index];
            scheduler->current_task_index = (scheduler->current_task_index + 1) % scheduler->num_tasks;
            pthread_mutex_unlock(&scheduler->lock);
            return task;
        }
        scheduler->current_task_index = (scheduler->current_task_index + 1) % scheduler->num_tasks;
    }
    for (int i = 0; i < scheduler->num_tasks; i++) {
        scheduler->tasks[i].remaining_quanta = scheduler->tasks[i].weight * scheduler->quanta_per_round;
    }
    pthread_mutex_unlock(&scheduler->lock);
    return __get_next_task(scheduler);
}

void __free_scheduler(Scheduler *scheduler) {
    free(scheduler->tasks);
    scheduler->tasks = NULL;
    scheduler->num_tasks = 0;
    scheduler->current_task_index = 0;
    pthread_mutex_destroy(&scheduler->lock);
}

void* worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool*) arg;
    while (1) {
        pthread_mutex_lock(&pool->queue_lock);
        while (pool->scheduler.num_tasks == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_lock);
        }
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_lock);
            pthread_exit(NULL);
        }
        Task *task = __get_next_task(&pool->scheduler);
        pthread_mutex_unlock(&pool->queue_lock);

        if (task != NULL) {
            task->function(task->arg);
        }
    }
    return NULL;
}

int thread_pool_init(ThreadPool *pool, int num_threads, int quanta_per_round) {
    pool->num_threads = num_threads;
    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (pool->threads == NULL) {
        perror("Failed to allocate threads");
        return -1;
    }
    __init_scheduler(&pool->scheduler, quanta_per_round);
    pthread_mutex_init(&pool->queue_lock, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);
    pool->shutdown = 0;
    return 0;
}

int thread_pool_add_task(ThreadPool *pool, void (*function)(void*), void *arg, int task_id, int weight) {
    __add_task(&pool->scheduler, task_id, weight, function, arg);
    pthread_cond_signal(&pool->queue_cond);
    return 0;
}

void thread_pool_start(ThreadPool *pool) {
    for (int i = 0; i < pool->num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }
}

void thread_pool_shutdown(ThreadPool *pool) {
    pthread_mutex_lock(&pool->queue_lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_lock);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    free(pool->threads);
    __free_scheduler(&pool->scheduler);
    pthread_mutex_destroy(&pool->queue_lock);
    pthread_cond_destroy(&pool->queue_cond);
}

