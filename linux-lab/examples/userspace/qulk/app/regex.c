#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define BUFFER_SIZE 4096

void match_mapped_files(const char *pattern) {
    FILE *file = fopen("/proc/self/maps", "r");
    if (!file) {
        perror("Failed to open /proc/self/maps");
        return;
    }

    regex_t regex;
    int ret = regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
        char errbuf[BUFFER_SIZE];
        regerror(ret, &regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "Regex compilation failed: %s\n", errbuf);
        fclose(file);
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        // Split the line to get the memory range, permissions, and file path (if present)
        unsigned long start, end;
        char perms[5], file_path[BUFFER_SIZE];
        int matched = sscanf(line, "%lx-%lx %4s %*s %*s %*s %[^\n]", &start, &end, perms, file_path);

        if (matched == 4) { // We have start, end, permissions, and file path
            // Execute regex match
            ret = regexec(&regex, file_path, 0, NULL, 0);
            if (!ret) {
                printf("File: %s\n", file_path);
                printf("Start Address: %lx\n", start);
                printf("End Address: %lx\n", end);
                printf("Size: %lx\n", end - start);
                printf("Permissions: %s\n\n", perms);
            } else if (ret != REG_NOMATCH) {
                char errbuf[BUFFER_SIZE];
                regerror(ret, &regex, errbuf, sizeof(errbuf));
                fprintf(stderr, "Regex match failed: %s\n", errbuf);
                break;
            }
        }
    }

    regfree(&regex);
    fclose(file);
}

int main(int argc, char *argv[]) {
    const char *pattern = ".*";  // Default pattern to match all paths

    if (argc == 2) {
        pattern = argv[1];
    }

    match_mapped_files(pattern);
    return EXIT_SUCCESS;
}

