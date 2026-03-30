#include <stdio.h>
#include <stdlib.h>
#include "drtc.h"

Scheduler* create_scheduler() {
    Scheduler *scheduler = (Scheduler*)malloc(sizeof(Scheduler));
    if (!scheduler) {
        return NULL;
    }
    scheduler->red_queue = NULL;
    scheduler->yellow_queue = NULL;
    scheduler->green_queue = NULL;
    scheduler->red_count = 0;
    scheduler->yellow_count = 0;
    scheduler->green_count = 0;
    return scheduler;
}

void drtc_add_task(Scheduler *scheduler, void (*function)(void *arg), void *arg, Priority priority) {
    Task *new_task = (Task*)malloc(sizeof(Task));
    if (!new_task) {
        return;
    }
    new_task->function = function;
    new_task->arg = arg;
    new_task->priority = priority;
    new_task->next = NULL;

    Task **queue;
    int *count;
    switch (priority) {
        case PRIORITY_RED:
            queue = &scheduler->red_queue;
            count = &scheduler->red_count;
            break;
        case PRIORITY_YELLOW:
            queue = &scheduler->yellow_queue;
            count = &scheduler->yellow_count;
            break;
        case PRIORITY_GREEN:
            queue = &scheduler->green_queue;
            count = &scheduler->green_count;
            break;
    }

    if (*queue == NULL) {
        *queue = new_task;
    } else {
        Task *current = *queue;
        while (current->next) {
            current = current->next;
        }
        current->next = new_task;
    }
    (*count)++;
}

void execute_tasks(Scheduler *scheduler) {
    int red_count = 0, yellow_count = 0, green_count = 0;

    while (scheduler->red_queue || scheduler->yellow_queue || scheduler->green_queue) {
        if (scheduler->red_queue && (red_count < 2 || !scheduler->yellow_queue || !scheduler->green_queue)) {
            Task *task = scheduler->red_queue;
            scheduler->red_queue = scheduler->red_queue->next;
            task->function(task->arg);
            free(task);
            red_count++;
            yellow_count = green_count = 0;
        } else if (scheduler->yellow_queue && (yellow_count < 1 || !scheduler->green_queue)) {
            Task *task = scheduler->yellow_queue;
            scheduler->yellow_queue = scheduler->yellow_queue->next;
            task->function(task->arg);
            free(task);
            yellow_count++;
            red_count = green_count = 0;
        } else if (scheduler->green_queue) {
            Task *task = scheduler->green_queue;
            scheduler->green_queue = scheduler->green_queue->next;
            task->function(task->arg);
            free(task);
            green_count++;
            red_count = yellow_count = 0;
        }
    }
}

void destroy_scheduler(Scheduler *scheduler) {
    Task *task;
    while (scheduler->red_queue) {
        task = scheduler->red_queue;
        scheduler->red_queue = scheduler->red_queue->next;
        free(task);
    }
    while (scheduler->yellow_queue) {
        task = scheduler->yellow_queue;
        scheduler->yellow_queue = scheduler->yellow_queue->next;
        free(task);
    }
    while (scheduler->green_queue) {
        task = scheduler->green_queue;
        scheduler->green_queue = scheduler->green_queue->next;
        free(task);
    }
    free(scheduler);
}

