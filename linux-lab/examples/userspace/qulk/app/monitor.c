#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define EVENT_BUF_LEN     (1024 * (EVENT_SIZE + 16))

// Function to get the username from UID
const char* get_username(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}

// Function to get PID of the process accessing the file
pid_t get_pid_from_fd(int fd) {
    char path[1024];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    char target[1024];
    ssize_t len = readlink(path, target, sizeof(target) - 1);
    if (len != -1) {
        target[len] = '\0';
        int pid;
        if (sscanf(target, "%*[^0-9]%d", &pid) == 1) {
            return (pid_t)pid;
        }
    }
    return -1;
}


void handle_events(int fd, int *wd, const char *target) {
    char buffer[EVENT_BUF_LEN];
    int length, i = 0;

    length = read(fd, buffer, EVENT_BUF_LEN); 

    if (length < 0) {
        perror("read");
        return;
    }  

    while (i < length) {     
        struct inotify_event *event = (struct inotify_event *) &buffer[i];

        if (event->len) {
            pid_t pid = get_pid_from_fd(fd);
            uid_t uid = geteuid(); // Get the effective user ID
            const char *username = get_username(uid);

            if (event->mask & IN_CREATE) {
                if (event->mask & IN_ISDIR) {
                    printf("User %s (PID %d) created directory %s\n", username, pid, event->name);
                } else {
                    printf("User %s (PID %d) created file %s\n", username, pid, event->name);
                }
            } else if (event->mask & IN_DELETE) {
                if (event->mask & IN_ISDIR) {
                    printf("User %s (PID %d) deleted directory %s\n", username, pid, event->name);
                } else {
                    printf("User %s (PID %d) deleted file %s\n", username, pid, event->name);
                }
            } else if (event->mask & IN_MODIFY) {
                if (event->mask & IN_ISDIR) {
                    printf("User %s (PID %d) modified directory %s\n", username, pid, event->name);
                } else {
                    printf("User %s (PID %d) wrote to file %s\n", username, pid, event->name);
                }
            } else if (event->mask & IN_ACCESS) {
                if (!(event->mask & IN_ISDIR)) {
                    printf("User %s (PID %d) read file %s\n", username, pid, event->name);
                }
            }
        }
        i += EVENT_SIZE + event->len;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *target = argv[1];

    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    int wd = inotify_add_watch(fd, target, IN_CREATE | IN_DELETE | IN_MODIFY | IN_ACCESS);
    if (wd == -1) {
        fprintf(stderr, "Cannot watch '%s': %s\n", target, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Watching %s for changes...\n", target);

    while (1) {
        handle_events(fd, &wd, target);
    }

    inotify_rm_watch(fd, wd);
    close(fd);

    return 0;
}

