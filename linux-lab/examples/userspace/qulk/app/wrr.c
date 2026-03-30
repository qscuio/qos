#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_QUEUES 1024
#define MAX_WEIGHT 1024


struct queue {
  int weight;
  pthread_t thread;
  void * (*function)(void *);
  void * data;
};

struct wrr_scheduler {
  struct queue queues[MAX_QUEUES];
  int max_queues;
  int num_queues;
};

void wrr_scheduler_init(struct wrr_scheduler *scheduler, int max_queues) {
  if (max_queues <= 0) {
    max_queues = 1;
  }
  scheduler->max_queues = max_queues;
  for (int i = 0; i < max_queues; i++) {
    scheduler->queues[i].weight = 1;
    scheduler->queues[i].thread = 0;
    scheduler->queues[i].function = 0;
    scheduler->queues[i].data = NULL;
  }

  return;
}

void wrr_scheduler_add_queue(struct wrr_scheduler *scheduler, int weight, void * (*function)(void *), void * data) {
  if (scheduler->num_queues == scheduler->max_queues) {
    return;
  }
  if (weight > MAX_WEIGHT) {
    weight = MAX_WEIGHT;
  }
  scheduler->queues[scheduler->num_queues].weight = weight;
  scheduler->queues[scheduler->num_queues].function = function;
  scheduler->queues[scheduler->num_queues].data = data;
  scheduler->num_queues++;

  return;
}

void wrr_scheduler_start(struct wrr_scheduler *scheduler) {
  int i;
  for (i = 0; i < scheduler->num_queues; i++) {
    int err = pthread_create(&scheduler->queues[i].thread, NULL, scheduler->queues[i].function, scheduler->queues[i].data);
    if (err != 0) {
      // Error creating thread
      return;
    }
  }
  return;
}

void wrr_scheduler_join(struct wrr_scheduler *scheduler) {
  int i;
  for (i = 0; i < scheduler->num_queues; i++) {
    pthread_join(scheduler->queues[i].thread, NULL);
  }
  return;
}

void *print_hello(void *data) {
  printf("Hello, world!\n");
  return NULL;
}

void *print_world(void *data) {
  printf("World!\n");
  return NULL;
}

int main() {
  struct wrr_scheduler scheduler;
  int num_queues = 2;
  wrr_scheduler_init(&scheduler, num_queues);
  wrr_scheduler_add_queue(&scheduler, 1, print_hello, NULL);
  wrr_scheduler_add_queue(&scheduler, 2, print_world, NULL);
  wrr_scheduler_start(&scheduler);
  wrr_scheduler_join(&scheduler);
  return 0;
}

