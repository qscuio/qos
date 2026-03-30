#include <stdio.h>
#include "gpt/wrr_scheduler.h"

int main() {
    Scheduler scheduler;
    init_scheduler(&scheduler, 1);

    add_task(&scheduler, 1, 4); // Task 1 with weight 3
    add_task(&scheduler, 2, 2); // Task 2 with weight 2
    add_task(&scheduler, 3, 1); // Task 3 with weight 1

    for (int i = 0; i < 16; i++) {
        int task_id = get_next_task(&scheduler);
        printf("Running task %d\n", task_id);
    }

    free_scheduler(&scheduler);
    return 0;
}
