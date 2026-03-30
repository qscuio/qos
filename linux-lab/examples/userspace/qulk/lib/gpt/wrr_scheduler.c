#include <stdio.h>
#include <stdlib.h>
#include "wrr_scheduler.h"

void init_scheduler(Scheduler *scheduler, int quanta_per_round) {
    scheduler->tasks = NULL;
    scheduler->num_tasks = 0;
    scheduler->current_task_index = 0;
    scheduler->quanta_per_round = quanta_per_round;
}

void add_task(Scheduler *scheduler, int task_id, int weight) {
    scheduler->tasks = realloc(scheduler->tasks, (scheduler->num_tasks + 1) * sizeof(Task));
    if (scheduler->tasks == NULL) {
        perror("Failed to add task");
        exit(EXIT_FAILURE);
    }
    scheduler->tasks[scheduler->num_tasks].task_id = task_id;
    scheduler->tasks[scheduler->num_tasks].weight = weight;
    scheduler->tasks[scheduler->num_tasks].remaining_quanta = weight * scheduler->quanta_per_round;
    scheduler->num_tasks++;
}

int get_next_task(Scheduler *scheduler) {
    for (int i = 0; i < scheduler->num_tasks; i++) {
        if (scheduler->tasks[scheduler->current_task_index].remaining_quanta > 0) {
            scheduler->tasks[scheduler->current_task_index].remaining_quanta--;
            return scheduler->tasks[scheduler->current_task_index].task_id;
        }
        scheduler->current_task_index = (scheduler->current_task_index + 1) % scheduler->num_tasks;
    }
    reset_quanta(scheduler);
    return get_next_task(scheduler);
}

void reset_quanta(Scheduler *scheduler) {
    for (int i = 0; i < scheduler->num_tasks; i++) {
        scheduler->tasks[i].remaining_quanta = scheduler->tasks[i].weight * scheduler->quanta_per_round;
    }
}

void free_scheduler(Scheduler *scheduler) {
    free(scheduler->tasks);
    scheduler->tasks = NULL;
    scheduler->num_tasks = 0;
    scheduler->current_task_index = 0;
}
