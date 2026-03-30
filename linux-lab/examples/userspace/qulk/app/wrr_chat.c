#include <stdio.h>
#include <stdlib.h>

// Structure to represent a task
typedef struct {
    int id;             // Task ID
    int weight;         // Task weight (relative to other tasks)
    int remaining_work; // Remaining work for the task
} Task;

// Structure to represent a queue
typedef struct {
    int weight;         // Queue weight
    int current_work;   // Current work being processed in the queue
    Task* current_task; // Current task being processed in the queue
} Queue;

// Structure to represent the WRR scheduler
typedef struct {
    int num_queues;     // Number of queues
    Queue* queues;      // Array of queues
    int current_queue;  // Index of the current queue being serviced
} WRRScheduler;

// Initializes the WRR scheduler with the specified number of queues and their weights
void wrr_scheduler_init(WRRScheduler* scheduler, int num_queues, int* queue_weights) {
    scheduler->num_queues = num_queues;
    scheduler->queues = (Queue*)malloc(num_queues * sizeof(Queue));

    for (int i = 0; i < num_queues; i++) {
        scheduler->queues[i].weight = queue_weights[i];
        scheduler->queues[i].current_work = 0;
        scheduler->queues[i].current_task = NULL;
    }

    scheduler->current_queue = 0;
}

// Enqueues a task into the WRR scheduler
void wrr_scheduler_enqueue(WRRScheduler* scheduler, Task* task) {
    int queue_idx = scheduler->current_queue;
    scheduler->current_queue = (scheduler->current_queue + 1) % scheduler->num_queues;

    Queue* queue = &scheduler->queues[queue_idx];
    queue->current_work += task->remaining_work;
    task->remaining_work = 0;

    if (queue->current_task == NULL) {
        queue->current_task = task;
    }
}

// Services the next task in the WRR scheduler
Task* wrr_scheduler_service(WRRScheduler* scheduler) {
    int num_queues = scheduler->num_queues;
    int queue_idx = scheduler->current_queue;
    int total_work = 0;

    // Find the next task to service based on the queue weights
    Task* next_task = NULL;
    for (int i = 0; i < num_queues; i++) {
        Queue* queue = &scheduler->queues[queue_idx];
        if (queue->current_task != NULL) {
            next_task = queue->current_task;
            break;
        }
        queue_idx = (queue_idx + 1) % num_queues;
    }

    if (next_task != NULL) {
        // Service the task and update the queue's state
        Queue* queue = &scheduler->queues[queue_idx];
        queue->current_work -= next_task->weight;
        next_task->remaining_work -= next_task->weight;

        if (next_task->remaining_work <= 0) {
            queue->current_task = NULL;
        }

        total_work += next_task->weight;
    }

    scheduler->current_queue = (queue_idx + 1) % num_queues;

    // Return the next task to be serviced
    return next_task;
}

int main() {
    WRRScheduler scheduler;
    int num_queues = 3;
    int queue_weights[] = {1, 2, 3};

    // Initialize the WRR scheduler with 3 queues and their weights
    wrr_scheduler_init(&scheduler, num_queues, queue_weights);

    // Create some tasks
    Task task1 = {1, 1, 5};
    Task task2 = {2, 2, 8};
    Task task3 = {3, 3, 4};

    // Enqueue the tasks into the scheduler
    wrr_scheduler_enqueue(&scheduler, &task1);
    wrr_scheduler_enqueue(&scheduler, &task2);
    wrr_scheduler_enqueue(&scheduler, &task3);

    // Service the tasks using the WRR scheduler
    while (1) {
        Task* task = wrr_scheduler_service(&scheduler);

        if (task == NULL) {
            // No tasks remaining
            break;
        }

        printf("Servicing Task %d, remain %d\n", task->id, task->remaining_work);

        if (task->remaining_work > 0) {
            // Task needs to be re-enqueued for further service
            wrr_scheduler_enqueue(&scheduler, task);
        }
    }

    return 0;
}
