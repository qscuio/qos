#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <libgen.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

void handle_signal(int signal) {
    printf("Exiting...\n");
    exit(0);
}

void create_backup_directory(const char *src_dir, char *backup_dir) {
    snprintf(backup_dir, 1024, "%s/../%s_backup", src_dir, src_dir);

    struct stat st = {0};
    if (stat(backup_dir, &st) == -1) {
        if (mkdir(backup_dir, 0700) != 0) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
}

void create_initial_backup(const char *src_dir, const char *backup_dir) {
    struct dirent *entry;
    DIR *dp = opendir(src_dir);
    if (dp == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG) {
            char source_path[512];
            char backup_path[512];
            snprintf(source_path, sizeof(source_path), "%s/%s", src_dir, entry->d_name);
            snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, entry->d_name);

            char cp_command[2048];
            snprintf(cp_command, sizeof(cp_command), "cp %s %s", source_path, backup_path);
            int ret = system(cp_command);
            if (ret == -1) {
                perror("system");
            }
        }
    }

    closedir(dp);
}

void print_diff(const char *old_version_path, const char *new_version_path) {
    char diff_command[2048];
    snprintf(diff_command, sizeof(diff_command), "diff -urNsa --color=always %s %s", old_version_path, new_version_path);

    FILE *fp = popen(diff_command, "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }

    pclose(fp);
}

void monitor_directory(const char *src_dir, const char *backup_dir) {
    int length, i = 0;
    int fd;
    int wd;
    char buffer[EVENT_BUF_LEN];

    // Initialize inotify
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
    }

    // Add watch on the directory
    wd = inotify_add_watch(fd, src_dir, IN_CREATE | IN_DELETE | IN_MODIFY);

    while (1) {
        length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
        }

        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    printf("New file %s created.\n", event->name);
                } else if (event->mask & IN_DELETE) {
                    printf("File %s deleted.\n", event->name);
                } else if (event->mask & IN_MODIFY) {
                    printf("File %s modified.\n", event->name);

                    char old_version_path[512];
                    char new_version_path[512];
                    snprintf(old_version_path, sizeof(old_version_path), "%s/%s", backup_dir, event->name);
                    snprintf(new_version_path, sizeof(new_version_path), "%s/%s", src_dir, event->name);

                    // Print the diff output
                    print_diff(old_version_path, new_version_path);

                    // Update old version
                    char cp_command[2048];
                    snprintf(cp_command, sizeof(cp_command), "cp %s %s", new_version_path, old_version_path);
                    int ret = system(cp_command);
                    if (ret == -1) {
                        perror("system");
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
        i = 0;
    }

    // Clean up
    inotify_rm_watch(fd, wd);
    close(fd);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory_to_monitor>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *src_dir = argv[1];
    char backup_dir[1024];

    // Handle signals to exit gracefully
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Create backup directory automatically
    create_backup_directory(src_dir, backup_dir);

    // Create initial backup of the directory
    create_initial_backup(src_dir, backup_dir);

    // Monitor the directory for changes
    monitor_directory(src_dir, backup_dir);

    return 0;
}

