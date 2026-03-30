#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct wrr_task {
    void *data;  // Task data
    struct wrr_task *next;
} wrr_task_t;

typedef struct wrr_queue {
    int weight;          // Queue weight
    int current_weight;  // Current position for round-robin
    int total_tasks;     // Number of tasks in the queue
    wrr_task_t *head;    // Head of the task list
    wrr_task_t *tail;    // Tail of the task list
    struct wrr_queue *next;
    pthread_mutex_t lock;  // Lock for thread safety
} wrr_queue_t;

typedef struct wrr_scheduler {
    wrr_queue_t *queues;  // Linked list of queues
    int total_weight;     // Total weight of all queues
} wrr_scheduler_t;

// Function declarations
wrr_scheduler_t* wrr_create_scheduler();
wrr_queue_t* wrr_create_queue(wrr_scheduler_t *scheduler, int weight);
void wrr_add_task(wrr_queue_t *queue, void *task);
void* wrr_schedule(wrr_scheduler_t *scheduler);
void wrr_destroy_queue(wrr_scheduler_t *scheduler, wrr_queue_t *queue);
void wrr_destroy_scheduler(wrr_scheduler_t *scheduler);


// Create a new scheduler
wrr_scheduler_t* wrr_create_scheduler() {
    wrr_scheduler_t *scheduler = (wrr_scheduler_t *)malloc(sizeof(wrr_scheduler_t));
    scheduler->queues = NULL;
    scheduler->total_weight = 0;
    return scheduler;
}

// Create a new queue with a specified weight
wrr_queue_t* wrr_create_queue(wrr_scheduler_t *scheduler, int weight) {
    wrr_queue_t *queue = (wrr_queue_t *)malloc(sizeof(wrr_queue_t));
    queue->weight = weight;
    queue->current_weight = weight;
    queue->total_tasks = 0;
    queue->head = queue->tail = NULL;
    pthread_mutex_init(&queue->lock, NULL);
    
    // Add queue to scheduler's list
    queue->next = scheduler->queues;
    scheduler->queues = queue;
    
    // Update total weight of scheduler
    scheduler->total_weight += weight;
    
    return queue;
}

// Add a task to a specific queue
void wrr_add_task(wrr_queue_t *queue, void *task) {
    wrr_task_t *new_task = (wrr_task_t *)malloc(sizeof(wrr_task_t));
    new_task->data = task;
    new_task->next = NULL;

    pthread_mutex_lock(&queue->lock);

    if (queue->tail == NULL) {
        queue->head = queue->tail = new_task;
    } else {
        queue->tail->next = new_task;
        queue->tail = new_task;
    }
    queue->total_tasks++;

    pthread_mutex_unlock(&queue->lock);
}

// WRR Scheduling Algorithm
void* wrr_schedule(wrr_scheduler_t *scheduler) {
    wrr_queue_t *queue = scheduler->queues;

    while (queue != NULL) {
        pthread_mutex_lock(&queue->lock);
        
        if (queue->total_tasks > 0 && queue->current_weight > 0) {
            // Get the first task from the queue
            wrr_task_t *task = queue->head;
            if (task != NULL) {
                queue->head = task->next;
                if (queue->head == NULL) {
                    queue->tail = NULL;
                }
                queue->total_tasks--;
                
                queue->current_weight--;  // Decrement the weight for this round
                
                void *task_data = task->data;
                free(task);  // Free the task structure
                
                pthread_mutex_unlock(&queue->lock);
                return task_data;  // Return the scheduled task
            }
        }
        
        pthread_mutex_unlock(&queue->lock);
        
        // Move to the next queue
        queue = queue->next;
        
        if (queue == NULL) {
            // Restart from the first queue and reset the weights
            queue = scheduler->queues;
            while (queue != NULL) {
                pthread_mutex_lock(&queue->lock);
                queue->current_weight = queue->weight;
                pthread_mutex_unlock(&queue->lock);
                queue = queue->next;
            }
            queue = scheduler->queues;
        }
    }
    
    return NULL;  // No task to schedule
}

// Destroy a queue and free its resources
void wrr_destroy_queue(wrr_scheduler_t *scheduler, wrr_queue_t *queue) {
    pthread_mutex_lock(&queue->lock);
    
    wrr_task_t *task = queue->head;
    while (task != NULL) {
        wrr_task_t *next_task = task->next;
        free(task);
        task = next_task;
    }
    
    // Remove queue from scheduler
    wrr_queue_t **indirect = &scheduler->queues;
    while (*indirect != queue) {
        indirect = &(*indirect)->next;
    }
    *indirect = queue->next;
    
    scheduler->total_weight -= queue->weight;
    pthread_mutex_unlock(&queue->lock);
    
    pthread_mutex_destroy(&queue->lock);
    free(queue);
}

// Destroy the scheduler and all its queues
void wrr_destroy_scheduler(wrr_scheduler_t *scheduler) {
    wrr_queue_t *queue = scheduler->queues;
    while (queue != NULL) {
        wrr_queue_t *next_queue = queue->next;
        wrr_destroy_queue(scheduler, queue);
        queue = next_queue;
    }
    free(scheduler);
}

int main() {
    wrr_scheduler_t *scheduler = wrr_create_scheduler();
    
    wrr_queue_t *queue1 = wrr_create_queue(scheduler, 3);
    wrr_queue_t *queue2 = wrr_create_queue(scheduler, 1);
    wrr_queue_t *queue3 = wrr_create_queue(scheduler, 2);

    // Add tasks to queue1
    wrr_add_task(queue1, "Task A1");
    wrr_add_task(queue1, "Task A2");
    wrr_add_task(queue1, "Task A3");
	wrr_add_task(queue1, "Task A4");
    wrr_add_task(queue1, "Task A5");
    wrr_add_task(queue1, "Task A6");
    
    
    // Add tasks to queue2
    wrr_add_task(queue2, "Task B1");
    wrr_add_task(queue2, "Task B2");
    wrr_add_task(queue2, "Task B3");

	// Add tasks to queue3
    wrr_add_task(queue3, "Task C1");
    wrr_add_task(queue3, "Task C2");
    wrr_add_task(queue3, "Task C3");
    wrr_add_task(queue3, "Task C4");

    for (int i = 0; i < 14; i++) {
        char *task = (char *)wrr_schedule(scheduler);
        if (task) {
            printf("Scheduled: %s\n", task);
        }
    }

    // Cleanup
    wrr_destroy_scheduler(scheduler);
    return 0;
}

