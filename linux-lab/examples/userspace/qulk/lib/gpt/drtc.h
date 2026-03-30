#ifndef DRTC_SCHEDULER_H
#define DRTC_SCHEDULER_H

typedef enum {
    PRIORITY_RED,
    PRIORITY_YELLOW,
    PRIORITY_GREEN
} Priority;

typedef struct Task {
    void (*function)(void *arg);
    void *arg;
    Priority priority;
    struct Task *next;
} Task;

typedef struct {
    Task *red_queue;
    Task *yellow_queue;
    Task *green_queue;
    int red_count;
    int yellow_count;
    int green_count;
} Scheduler;

// Function to create a new scheduler
Scheduler* create_scheduler();

// Function to add a task to the scheduler
void drtc_add_task(Scheduler *scheduler, void (*function)(void *arg), void *arg, Priority priority);

// Function to execute tasks based on the DRTC algorithm
void execute_tasks(Scheduler *scheduler);

// Function to destroy the scheduler and free resources
void destroy_scheduler(Scheduler *scheduler);

#endif // DRTC_SCHEDULER_H

