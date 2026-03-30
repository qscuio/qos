#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    void (*function)(void *arg);
    void *argument;
} task_t;


void *threadpool_worker(void *arg);

typedef struct {
    task_t *task_queue;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_not_empty;
    pthread_t *threads;
    int num_threads;
    int queue_size;
    int front;
    int rear;
    int count;
    int shutdown;
} threadpool_t;

void threadpool_init(threadpool_t *pool, int num_threads, int queue_size) {
    pool->num_threads = num_threads;
    pool->queue_size = queue_size;
    pool->front = 0;
    pool->rear = 0;
    pool->count = 0;
    pool->shutdown = 0;

    pool->task_queue = (task_t *)malloc(queue_size * sizeof(task_t));
    pool->threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));

    pthread_mutex_init(&(pool->queue_lock), NULL);
    pthread_cond_init(&(pool->queue_not_empty), NULL);

    for (int i = 0; i < num_threads; i++) {
	pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool);
    }
}

void threadpool_submit(threadpool_t *pool, void (*function)(void *arg), void *argument) {
    pthread_mutex_lock(&(pool->queue_lock));

    while (pool->count == pool->queue_size && !pool->shutdown) {
	pthread_cond_wait(&(pool->queue_not_empty), &(pool->queue_lock));
    }

    if (pool->shutdown) {
	pthread_mutex_unlock(&(pool->queue_lock));
	return;
    }

    pool->task_queue[pool->rear].function = function;
    pool->task_queue[pool->rear].argument = argument;
    pool->rear = (pool->rear + 1) % pool->queue_size;
    pool->count++;

    pthread_cond_signal(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->queue_lock));
}

void *threadpool_worker(void *arg) {
    threadpool_t *pool = (threadpool_t *)arg;
    task_t task;

    while (1) {
	pthread_mutex_lock(&(pool->queue_lock));

	while (pool->count == 0 && !pool->shutdown) {
	    pthread_cond_wait(&(pool->queue_not_empty), &(pool->queue_lock));
	}

	if (pool->shutdown) {
	    pthread_mutex_unlock(&(pool->queue_lock));
	    pthread_exit(NULL);
	}

	task.function = pool->task_queue[pool->front].function;
	task.argument = pool->task_queue[pool->front].argument;
	pool->front = (pool->front + 1) % pool->queue_size;
	pool->count--;

	pthread_cond_broadcast(&(pool->queue_not_empty));
	pthread_mutex_unlock(&(pool->queue_lock));

	(*(task.function))(task.argument);
    }

    pthread_exit(NULL);
}

void threadpool_shutdown(threadpool_t *pool) {
    pthread_mutex_lock(&(pool->queue_lock));
    pool->shutdown = 1;
    pthread_mutex_unlock(&(pool->queue_lock));

    pthread_cond_broadcast(&(pool->queue_not_empty));

    for (int i = 0; i < pool->num_threads; i++) {
	pthread_join(pool->threads[i], NULL);
    }

    free(pool->task_queue);

    free(pool->threads);
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_not_empty));
}

// Example usage

void print_message(void *arg) {
    char *message = (char *)arg;
    printf("%s\n", message);
}

int main() {
    threadpool_t pool;
    int num_threads = 4;
    int queue_size = 10;
    threadpool_init(&pool, num_threads, queue_size);

    // Submit tasks to the thread pool
    threadpool_submit(&pool, print_message, "Task 1");
    threadpool_submit(&pool, print_message, "Task 2");
    threadpool_submit(&pool, print_message, "Task 3");
    threadpool_submit(&pool, print_message, "Task 4");

    // Wait for tasks to complete
    sleep(1);

    // Shutdown the thread pool
    threadpool_shutdown(&pool);

    return 0;
}

// 
// In this example, the `threadpool_t` structure represents the thread pool and contains the necessary components such as the task queue, mutexes, condition variables, and thread IDs. The `threadpool_init` function initializes the thread pool with the desired number of threads and queue size. The `threadpool_submit` function allows you to submit tasks to the pool, and the `print_message` function is an example task that simply prints a message.
//
// The `threadpool_worker` function is the entry point for each worker thread. It continuously checks for tasks in the task queue and executes them. The `threadpool_shutdown` function gracefully shuts down the thread pool by setting the `shutdown` flag, signaling the worker threads to exit, and joining them.
//
// In the example `main` function, we initialize the thread pool, submit some tasks, wait for them to complete, and then shutdown the pool.
//
// Please note that this is a basic implementation, and in a real-world scenario, you may need to handle additional scenarios such as error checking, task dependencies, and resource cleanup.
//
