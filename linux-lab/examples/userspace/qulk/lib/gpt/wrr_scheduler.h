#ifndef WRR_SCHEDULER_H
#define WRR_SCHEDULER_H

typedef struct {
    int task_id;
    int weight;
    int remaining_quanta;
} Task;

typedef struct {
    Task *tasks;
    int num_tasks;
    int current_task_index;
    int quanta_per_round;
} Scheduler;

// Initialize the scheduler
void init_scheduler(Scheduler *scheduler, int quanta_per_round);

// Add a task to the scheduler
void add_task(Scheduler *scheduler, int task_id, int weight);

// Get the next task to run based on WRR algorithm
int get_next_task(Scheduler *scheduler);

// Reset the remaining quanta for the tasks for the next round
void reset_quanta(Scheduler *scheduler);

// Free the resources allocated by the scheduler
void free_scheduler(Scheduler *scheduler);

#endif // WRR_SCHEDULER_H

