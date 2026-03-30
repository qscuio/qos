// threadpool.c
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

typedef struct threadpool_task_t {
    void (*function)(void *);          // 任务函数
    void *argument;                    // 参数
    struct threadpool_task_t *next;    // 链表指针
} threadpool_task_t;

typedef struct {
    pthread_mutex_t lock;         // 互斥锁保护共享数据
    pthread_cond_t notify;        // 条件变量用于通知线程
    pthread_t *threads;           // 工作线程数组
    threadpool_task_t *task_queue_head;  // 任务队列头指针
    threadpool_task_t *task_queue_tail;  // 任务队列尾指针
    int thread_count;             // 当前线程数
    int shutdown;                 // 关闭标志，非0时停止线程
    int threads_to_exit;          // 待退出线程数量（用于动态调整减少线程数）
} threadpool_t;

/* API 声明 */
threadpool_t *threadpool_create(int thread_count);
int threadpool_destroy(threadpool_t *pool);
int threadpool_add_task(threadpool_t *pool, void (*function)(void *), void *argument);
int threadpool_set_thread_count(threadpool_t *pool, int new_thread_count);

/* 内部工作线程函数 */
static void *threadpool_worker(void *arg) {
    threadpool_t *pool = (threadpool_t *)arg;
    threadpool_task_t *task;

    while (1) {
        // 加锁等待任务到来或退出信号
        pthread_mutex_lock(&(pool->lock));

        // 如果有待退出标记且任务队列为空，退出线程
        if (pool->threads_to_exit > 0 && pool->task_queue_head == NULL) {
            pool->threads_to_exit--;
            pool->thread_count--;
            pthread_mutex_unlock(&(pool->lock));
            // 线程退出前释放线程资源
            pthread_exit(NULL);
        }

        // 等待任务到来或者关闭信号
        while (pool->task_queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
            // 再次检查待退出标记
            if (pool->threads_to_exit > 0 && pool->task_queue_head == NULL) {
                pool->threads_to_exit--;
                pool->thread_count--;
                pthread_mutex_unlock(&(pool->lock));
                pthread_exit(NULL);
            }
        }

        // 如果线程池正在关闭，则退出线程
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // 从任务队列中取出任务
        task = pool->task_queue_head;
        if (task) {
            pool->task_queue_head = task->next;
            if (pool->task_queue_head == NULL) {
                pool->task_queue_tail = NULL;
            }
        }
        pthread_mutex_unlock(&(pool->lock));

        // 执行任务
        if (task) {
            task->function(task->argument);
            free(task);
        }
    }
    return NULL;
}

/* 创建线程池 */
threadpool_t *threadpool_create(int thread_count) {
    if (thread_count <= 0) {
        thread_count = 1;
    }

    threadpool_t *pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (!pool) {
        perror("Failed to allocate threadpool");
        return NULL;
    }

    // 初始化结构体
    pool->thread_count = 0;
    pool->shutdown = 0;
    pool->threads_to_exit = 0;
    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;

    if (pthread_mutex_init(&(pool->lock), NULL) != 0 ||
        pthread_cond_init(&(pool->notify), NULL) != 0) {
        perror("Failed to init mutex or condition variable");
        free(pool);
        return NULL;
    }

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if (!pool->threads) {
        perror("Failed to allocate thread array");
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
        free(pool);
        return NULL;
    }

    // 创建初始工作线程
    for (int i = 0; i < thread_count; ++i) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            perror("Failed to create thread");
            threadpool_destroy(pool);
            return NULL;
        }
        pool->thread_count++;
    }

    return pool;
}

/* 向线程池添加任务 */
int threadpool_add_task(threadpool_t *pool, void (*function)(void *), void *argument) {
    if (!pool || !function) {
        return -1;
    }

    threadpool_task_t *task = (threadpool_task_t *)malloc(sizeof(threadpool_task_t));
    if (!task) {
        perror("Failed to allocate task");
        return -1;
    }
    task->function = function;
    task->argument = argument;
    task->next = NULL;

    pthread_mutex_lock(&(pool->lock));
    // 将任务插入队列尾部
    if (pool->task_queue_tail) {
        pool->task_queue_tail->next = task;
        pool->task_queue_tail = task;
    } else {
        pool->task_queue_head = pool->task_queue_tail = task;
    }
    // 通知等待的线程有新任务
    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

/* 动态调整线程数 */
int threadpool_set_thread_count(threadpool_t *pool, int new_thread_count) {
    if (!pool || new_thread_count <= 0) {
        return -1;
    }

    pthread_mutex_lock(&(pool->lock));
    int current = pool->thread_count;
    if (new_thread_count == current) {
        // 无需调整
        pthread_mutex_unlock(&(pool->lock));
        return 0;
    } else if (new_thread_count > current) {
        // 增加线程：分配新的线程数组并创建新线程
        int add = new_thread_count - current;
        pthread_t *new_threads = realloc(pool->threads, sizeof(pthread_t) * new_thread_count);
        if (!new_threads) {
            pthread_mutex_unlock(&(pool->lock));
            perror("Failed to reallocate threads array");
            return -1;
        }
        pool->threads = new_threads;
        // 创建新增的线程（数组后续位置）
        for (int i = current; i < new_thread_count; i++) {
            if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
                perror("Failed to create new thread");
                // 如果创建失败，可考虑继续或回滚，此处简单返回错误
                pthread_mutex_unlock(&(pool->lock));
                return -1;
            }
            pool->thread_count++;
        }
    } else {
        // 减少线程：设置待退出标志，并唤醒所有线程
        int reduce = current - new_thread_count;
        pool->threads_to_exit += reduce;
        // 唤醒所有空闲线程，让其检查退出标记
        pthread_cond_broadcast(&(pool->notify));
    }
    pthread_mutex_unlock(&(pool->lock));
    return 0;
}

/* 销毁线程池 */
int threadpool_destroy(threadpool_t *pool) {
    if (!pool) {
        return -1;
    }

    pthread_mutex_lock(&(pool->lock));
    pool->shutdown = 1;
    // 唤醒所有等待线程
    pthread_cond_broadcast(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    // 等待所有线程结束
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // 清空任务队列
    while (pool->task_queue_head != NULL) {
        threadpool_task_t *task = pool->task_queue_head;
        pool->task_queue_head = task->next;
        free(task);
    }

    // 销毁互斥锁和条件变量
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));

    // 释放线程数组及线程池结构体
    free(pool->threads);
    free(pool);

    return 0;
}

//////////////////////
// 示例用法
//////////////////////

#include <time.h>

// 示例任务函数：打印任务编号和睡眠指定秒数
void example_task(void *arg) {
    int task_num = *(int *)arg;
    printf("Task %d started on thread %ld\n", task_num, pthread_self());
    // 模拟任务耗时
    sleep(1);
    printf("Task %d finished on thread %ld\n", task_num, pthread_self());
}

int main() {
    // 创建一个初始工作线程数为 3 的线程池
    threadpool_t *pool = threadpool_create(3);
    if (!pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        return -1;
    }

    const int num_tasks = 10;
    int task_args[num_tasks];
    // 提交 10 个任务
    for (int i = 0; i < num_tasks; i++) {
        task_args[i] = i + 1;
        threadpool_add_task(pool, example_task, &task_args[i]);
    }

    // 睡眠 2 秒后，增加线程数至 5
    sleep(2);
    printf("Increasing threads to 5\n");
    threadpool_set_thread_count(pool, 5);

    // 再提交 5 个任务
    for (int i = 0; i < 5; i++) {
        int task_id = num_tasks + i + 1;
        threadpool_add_task(pool, example_task, &task_id);
        // 为避免传入地址被覆盖，这里简单 sleep 一下
        usleep(10000);
    }

    // 睡眠 3 秒后，减少线程数至 2
    sleep(3);
    printf("Decreasing threads to 2\n");
    threadpool_set_thread_count(pool, 2);

    // 等待所有任务执行完毕
    sleep(5);

    // 销毁线程池
    threadpool_destroy(pool);
    printf("Thread pool destroyed.\n");

    return 0;
}

