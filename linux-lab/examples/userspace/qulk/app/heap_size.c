#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define PROC_DIR "/proc/"
#define MAX_PATH 256

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
        perror("fopen");
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

int main() {
    struct dirent *entry;
    DIR *proc_dir = opendir(PROC_DIR);

    if (!proc_dir) {
        perror("opendir");
        return 1;
    }

    size_t total_heap_size = 0;

    // Iterate over each entry in /proc to find PIDs
    while ((entry = readdir(proc_dir)) != NULL) {
        if (is_pid_dir(entry->d_name)) {
            size_t heap_size = get_heap_size_for_pid(entry->d_name);
            if (heap_size > 0) {
                printf("PID %s has heap size: %zu bytes\n", entry->d_name, heap_size);
                total_heap_size += heap_size;
            }
        }
    }

    closedir(proc_dir);

    printf("\nTotal heap size for all running processes: %zu bytes\n", total_heap_size);
    return 0;
}

