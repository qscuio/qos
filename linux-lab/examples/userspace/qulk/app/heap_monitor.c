#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

#define PROC_DIR "/proc/"
#define MAX_PATH 256
#define SLEEP_INTERVAL 30  // in seconds

// Struct to hold PID and its heap size
typedef struct {
    char pid[32];
    size_t heap_size;
} ProcessHeapInfo;

// Function to check if a directory name is a number (PID)
int is_pid_dir(const char *name) {
    while (*name) {
        if (*name < '0' || *name > '9') {
            return 0;
        }
        name++;
    }
    return 1;
}

// Function to calculate the heap size of a process given its PID
size_t get_heap_size_for_pid(const char *pid) {
    char maps_path[MAX_PATH];
    snprintf(maps_path, sizeof(maps_path), PROC_DIR "%s/maps", pid);

    FILE *maps_file = fopen(maps_path, "r");
    if (!maps_file) {
        return 0;
    }

    size_t heap_size = 0;
    char line[512];
    while (fgets(line, sizeof(line), maps_file)) {
        unsigned long start, end;
        char perms[5], path[256];

        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %s", &start, &end, perms, path) == 4) {
            if (strcmp(path, "[heap]") == 0) {
                heap_size += (end - start);
            }
        }
    }

    fclose(maps_file);
    return heap_size;
}

// Function to gather heap information for all processes
int gather_heap_info(ProcessHeapInfo *heap_info, size_t *total_heap_size) {
    struct dirent *entry;
    DIR *proc_dir = opendir(PROC_DIR);
    if (!proc_dir) {
        perror("opendir");
        return -1;
    }

    size_t index = 0;
    *total_heap_size = 0;

    // Iterate over each entry in /proc to find PIDs
    while ((entry = readdir(proc_dir)) != NULL) {
        if (is_pid_dir(entry->d_name)) {
            size_t heap_size = get_heap_size_for_pid(entry->d_name);
            if (heap_size > 0) {
                strcpy(heap_info[index].pid, entry->d_name);
                heap_info[index].heap_size = heap_size;
                *total_heap_size += heap_size;
                index++;
            }
        }
    }

    closedir(proc_dir);
    return index; // Return number of processes with heap info
}

int main() {
    ProcessHeapInfo previous_heap_info[1024] = {0}; // Array to hold the previous heap info
    size_t previous_total_heap_size = 0;
    int num_previous_processes = 0;

    while (1) {
        printf("\nGathering heap info...\n");

        ProcessHeapInfo current_heap_info[1024] = {0}; // Array to hold current heap info
        size_t current_total_heap_size = 0;
        int num_current_processes = gather_heap_info(current_heap_info, &current_total_heap_size);

        // Print current heap size for all processes
        for (int i = 0; i < num_current_processes; i++) {
            printf("PID %s has heap size: %zu bytes\n", current_heap_info[i].pid, current_heap_info[i].heap_size);
        }

        printf("Total heap size for all running processes: %zu -- %zu bytes\n", previous_total_heap_size, current_total_heap_size);

        // Compare with previous iteration
        if (num_previous_processes > 0) {
            for (int i = 0; i < num_current_processes; i++) {
                for (int j = 0; j < num_previous_processes; j++) {
                    if (strcmp(current_heap_info[i].pid, previous_heap_info[j].pid) == 0) {
                        if (current_heap_info[i].heap_size > previous_heap_info[j].heap_size) {
                            printf("PID %s heap size increased by %zu bytes\n",
                                   current_heap_info[i].pid,
                                   current_heap_info[i].heap_size - previous_heap_info[j].heap_size);
                        }
                        break;
                    }
                }
            }
        }

        // Copy current heap info to previous for the next iteration
        memcpy(previous_heap_info, current_heap_info, sizeof(current_heap_info));
        previous_total_heap_size = current_total_heap_size;
        num_previous_processes = num_current_processes;

        // Sleep for the defined interval
        sleep(SLEEP_INTERVAL);
    }

    return 0;
}

