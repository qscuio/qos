/* 
 * Hierarchical Database with Subtree Subscription
 * 编译: gcc -o test_multi test_multi.c -lpthread -lrt
 * 
 * 支持层级路径订阅，例如:
 * - "interfaces" 订阅所有接口
 * - "interfaces/eth0" 订阅特定接口
 * - "interfaces/eth0/status" 订阅特定状态
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/* 配置常量 */
#define DB_NAME "/global_db_hierarchical"
#define MAX_KEYS 1024
#define MAX_KEY_LEN 256
#define MAX_VALUE_LEN 512
#define MAX_SUBSCRIBERS 32
#define PATH_SEPARATOR '/'

/* 数据库条目结构 */
typedef struct {
    char path[MAX_KEY_LEN];      /* 层级路径: interfaces/eth0/status */
    char value[MAX_VALUE_LEN];
    int valid;
    uint64_t version;
    time_t last_modified;
} db_entry_t;

/* 订阅者信息 */
typedef struct {
    char subtree[MAX_KEY_LEN];   /* 订阅的子树路径 */
    int active;
    pid_t pid;
    int exact_match;             /* 1=精确匹配, 0=子树匹配 */
} subscriber_info_t;

/* 通知队列条目 */
typedef struct {
    char path[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    int valid;
} notify_entry_t;

/* 全局数据库结构 */
typedef struct {
    pthread_rwlock_t lock;
    db_entry_t entries[MAX_KEYS];
    int entry_count;
    
    /* 订阅者管理 */
    subscriber_info_t subscribers[MAX_SUBSCRIBERS];
    pthread_mutex_t sub_lock;
    
    /* 通知队列（每个订阅者一个队列） */
    notify_entry_t notify_queue[MAX_SUBSCRIBERS][16];
    volatile int notify_head[MAX_SUBSCRIBERS];
    volatile int notify_tail[MAX_SUBSCRIBERS];
    
    /* 统计信息 */
    uint64_t read_count;
    uint64_t write_count;
    uint64_t notify_count;
} global_db_t;

/* 数据库句柄 */
typedef struct {
    global_db_t *db;
    int shm_fd;
    int is_creator;
    int my_sub_slot;
} db_handle_t;

/* ========== 路径匹配函数 ========== */

/* 检查 path 是否在 subtree 下 */
static int path_is_under_subtree(const char *path, const char *subtree) {
    size_t subtree_len = strlen(subtree);
    
    if (subtree_len == 0) return 1;  /* 空路径匹配所有 */
    
    /* 精确匹配 */
    if (strcmp(path, subtree) == 0) return 1;
    
    /* 检查是否是子路径 */
    if (strncmp(path, subtree, subtree_len) == 0) {
        /* 确保是完整的路径组件（不是部分匹配） */
        if (path[subtree_len] == PATH_SEPARATOR || path[subtree_len] == '\0') {
            return 1;
        }
    }
    
    return 0;
}

/* 规范化路径（移除尾部斜杠） */
static void normalize_path(char *path) {
    size_t len = strlen(path);
    while (len > 0 && path[len - 1] == PATH_SEPARATOR) {
        path[len - 1] = '\0';
        len--;
    }
}

/* ========== 核心数据库函数 ========== */

/* 创建或打开全局数据库 */
db_handle_t* db_open(int create) {
    db_handle_t *handle = malloc(sizeof(db_handle_t));
    if (!handle) return NULL;
    
    handle->is_creator = create;
    handle->my_sub_slot = -1;
    
    int flags = O_RDWR;
    if (create) {
        shm_unlink(DB_NAME);
        flags |= O_CREAT;
        handle->shm_fd = shm_open(DB_NAME, flags, 0666);
    } else {
        handle->shm_fd = shm_open(DB_NAME, flags, 0666);
    }
    
    if (handle->shm_fd == -1) {
        perror("shm_open");
        free(handle);
        return NULL;
    }
    
    if (handle->is_creator) {
        if (ftruncate(handle->shm_fd, sizeof(global_db_t)) == -1) {
            perror("ftruncate");
            close(handle->shm_fd);
            free(handle);
            return NULL;
        }
    }
    
    handle->db = mmap(NULL, sizeof(global_db_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED, 
                      handle->shm_fd, 0);
    
    if (handle->db == MAP_FAILED) {
        perror("mmap");
        close(handle->shm_fd);
        free(handle);
        return NULL;
    }
    
    if (handle->is_creator) {
        pthread_rwlockattr_t rwlock_attr;
        pthread_rwlockattr_init(&rwlock_attr);
        pthread_rwlockattr_setpshared(&rwlock_attr, PTHREAD_PROCESS_SHARED);
        pthread_rwlock_init(&handle->db->lock, &rwlock_attr);
        
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&handle->db->sub_lock, &mutex_attr);
        
        memset(handle->db->entries, 0, sizeof(handle->db->entries));
        memset(handle->db->subscribers, 0, sizeof(handle->db->subscribers));
        memset(handle->db->notify_queue, 0, sizeof(handle->db->notify_queue));
        memset((void*)handle->db->notify_head, 0, sizeof(handle->db->notify_head));
        memset((void*)handle->db->notify_tail, 0, sizeof(handle->db->notify_tail));
        handle->db->entry_count = 0;
        handle->db->read_count = 0;
        handle->db->write_count = 0;
        handle->db->notify_count = 0;
    }
    
    return handle;
}

void db_close(db_handle_t *handle) {
    if (!handle) return;
    
    if (handle->my_sub_slot >= 0) {
        pthread_mutex_lock(&handle->db->sub_lock);
        handle->db->subscribers[handle->my_sub_slot].active = 0;
        pthread_mutex_unlock(&handle->db->sub_lock);
    }
    
    munmap(handle->db, sizeof(global_db_t));
    close(handle->shm_fd);
    
    if (handle->is_creator) {
        shm_unlink(DB_NAME);
    }
    
    free(handle);
}

static int find_entry(global_db_t *db, const char *path) {
    int i;
    for (i = 0; i < db->entry_count; i++) {
        if (db->entries[i].valid && strcmp(db->entries[i].path, path) == 0) {
            return i;
        }
    }
    return -1;
}

/* 写入数据到指定路径 */
int db_set(db_handle_t *handle, const char *path, const char *value) {
    int i, idx;
    char normalized_path[MAX_KEY_LEN];
    
    if (!handle || !path || !value) return -1;
    
    strncpy(normalized_path, path, MAX_KEY_LEN - 1);
    normalized_path[MAX_KEY_LEN - 1] = '\0';
    normalize_path(normalized_path);
    
    pthread_rwlock_wrlock(&handle->db->lock);
    
    idx = find_entry(handle->db, normalized_path);
    
    if (idx == -1) {
        if (handle->db->entry_count >= MAX_KEYS) {
            pthread_rwlock_unlock(&handle->db->lock);
            return -1;
        }
        idx = handle->db->entry_count++;
        strncpy(handle->db->entries[idx].path, normalized_path, MAX_KEY_LEN - 1);
        handle->db->entries[idx].valid = 1;
        handle->db->entries[idx].version = 0;
    }
    
    strncpy(handle->db->entries[idx].value, value, MAX_VALUE_LEN - 1);
    handle->db->entries[idx].version++;
    handle->db->entries[idx].last_modified = time(NULL);
    handle->db->write_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    /* 通知匹配的订阅者 */
    pthread_mutex_lock(&handle->db->sub_lock);
    for (i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (handle->db->subscribers[i].active) {
            int should_notify = 0;
            
            if (handle->db->subscribers[i].exact_match) {
                should_notify = (strcmp(normalized_path, 
                                       handle->db->subscribers[i].subtree) == 0);
            } else {
                should_notify = path_is_under_subtree(normalized_path, 
                                                     handle->db->subscribers[i].subtree);
            }
            
            if (should_notify) {
                int head = handle->db->notify_head[i];
                int next_head = (head + 1) % 16;
                
                if (next_head != handle->db->notify_tail[i]) {
                    strncpy(handle->db->notify_queue[i][head].path, 
                           normalized_path, MAX_KEY_LEN - 1);
                    strncpy(handle->db->notify_queue[i][head].value, 
                           value, MAX_VALUE_LEN - 1);
                    handle->db->notify_queue[i][head].valid = 1;
                    handle->db->notify_head[i] = next_head;
                    handle->db->notify_count++;
                }
            }
        }
    }
    pthread_mutex_unlock(&handle->db->sub_lock);
    
    return 0;
}

/* 读取指定路径的数据 */
int db_get(db_handle_t *handle, const char *path, char *value, size_t value_len) {
    int idx;
    char normalized_path[MAX_KEY_LEN];
    
    if (!handle || !path || !value) return -1;
    
    strncpy(normalized_path, path, MAX_KEY_LEN - 1);
    normalized_path[MAX_KEY_LEN - 1] = '\0';
    normalize_path(normalized_path);
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    idx = find_entry(handle->db, normalized_path);
    if (idx == -1) {
        pthread_rwlock_unlock(&handle->db->lock);
        return -1;
    }
    
    strncpy(value, handle->db->entries[idx].value, value_len - 1);
    value[value_len - 1] = '\0';
    handle->db->read_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    return 0;
}

/* 列出子树下的所有条目 */
int db_list_subtree(db_handle_t *handle, const char *subtree, 
                    char paths[][MAX_KEY_LEN], int max_paths) {
    int i, count = 0;
    char normalized_subtree[MAX_KEY_LEN];
    
    if (!handle || !paths) return -1;
    
    strncpy(normalized_subtree, subtree, MAX_KEY_LEN - 1);
    normalized_subtree[MAX_KEY_LEN - 1] = '\0';
    normalize_path(normalized_subtree);
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    for (i = 0; i < handle->db->entry_count && count < max_paths; i++) {
        if (handle->db->entries[i].valid) {
            if (path_is_under_subtree(handle->db->entries[i].path, normalized_subtree)) {
                strncpy(paths[count], handle->db->entries[i].path, MAX_KEY_LEN - 1);
                count++;
            }
        }
    }
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    return count;
}

/* 订阅子树 */
int db_subscribe(db_handle_t *handle, const char *subtree, int exact_match) {
    int i;
    int slot = -1;
    char normalized_subtree[MAX_KEY_LEN];
    
    if (!handle || !subtree) return -1;
    
    strncpy(normalized_subtree, subtree, MAX_KEY_LEN - 1);
    normalized_subtree[MAX_KEY_LEN - 1] = '\0';
    normalize_path(normalized_subtree);
    
    pthread_mutex_lock(&handle->db->sub_lock);
    
    for (i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (!handle->db->subscribers[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        pthread_mutex_unlock(&handle->db->sub_lock);
        return -1;
    }
    
    strncpy(handle->db->subscribers[slot].subtree, normalized_subtree, MAX_KEY_LEN - 1);
    handle->db->subscribers[slot].active = 1;
    handle->db->subscribers[slot].pid = getpid();
    handle->db->subscribers[slot].exact_match = exact_match;
    handle->db->notify_head[slot] = 0;
    handle->db->notify_tail[slot] = 0;
    
    pthread_mutex_unlock(&handle->db->sub_lock);
    
    handle->my_sub_slot = slot;
    return slot;
}

/* 轮询通知 */
int db_poll_notification(db_handle_t *handle, char *path, char *value) {
    int tail;
    
    if (!handle || handle->my_sub_slot < 0) return 0;
    
    tail = handle->db->notify_tail[handle->my_sub_slot];
    
    if (tail != handle->db->notify_head[handle->my_sub_slot]) {
        if (handle->db->notify_queue[handle->my_sub_slot][tail].valid) {
            strcpy(path, handle->db->notify_queue[handle->my_sub_slot][tail].path);
            strcpy(value, handle->db->notify_queue[handle->my_sub_slot][tail].value);
            handle->db->notify_queue[handle->my_sub_slot][tail].valid = 0;
            handle->db->notify_tail[handle->my_sub_slot] = (tail + 1) % 16;
            return 1;
        }
    }
    
    return 0;
}

void db_print_stats(db_handle_t *handle) {
    if (!handle) return;
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    printf("\n=== Database Statistics ===\n");
    printf("Entries: %d/%d\n", handle->db->entry_count, MAX_KEYS);
    printf("Reads: %llu\n", (unsigned long long)handle->db->read_count);
    printf("Writes: %llu\n", (unsigned long long)handle->db->write_count);
    printf("Notifications: %llu\n", (unsigned long long)handle->db->notify_count);
    
    pthread_rwlock_unlock(&handle->db->lock);
}

/* ========== 测试程序 ========== */

void run_server() {
    int counter = 0;
    char path[MAX_KEY_LEN], value[MAX_VALUE_LEN];
    
    printf("=== Server Process (PID: %d) ===\n", getpid());
    printf("Creating hierarchical database...\n\n");
    
    db_handle_t *db = db_open(1);
    if (!db) {
        fprintf(stderr, "Failed to create database\n");
        return;
    }
    
    printf("Server running. Updating network interfaces...\n");
    printf("Database structure:\n");
    printf("  interfaces/\n");
    printf("    eth0/\n");
    printf("      status, speed, packets_tx, packets_rx\n");
    printf("    eth1/\n");
    printf("      status, speed, packets_tx, packets_rx\n");
    printf("  system/\n");
    printf("    hostname, uptime\n");
    printf("\nPress Ctrl+C to stop.\n\n");
    
    while (1) {
        /* 更新 eth0 接口 */
        snprintf(value, sizeof(value), "%s", counter % 2 ? "up" : "down");
        db_set(db, "interfaces/eth0/status", value);
        
        snprintf(value, sizeof(value), "%d", 1000 + (counter % 100));
        db_set(db, "interfaces/eth0/speed", value);
        
        snprintf(value, sizeof(value), "%d", counter * 100);
        db_set(db, "interfaces/eth0/packets_tx", value);
        
        /* 更新 eth1 接口 */
        snprintf(value, sizeof(value), "up");
        db_set(db, "interfaces/eth1/status", value);
        
        snprintf(value, sizeof(value), "%d", counter * 50);
        db_set(db, "interfaces/eth1/packets_tx", value);
        
        /* 更新系统信息 */
        snprintf(value, sizeof(value), "router-%03d", counter % 1000);
        db_set(db, "system/hostname", value);
        
        snprintf(value, sizeof(value), "%d", counter * 60);
        db_set(db, "system/uptime", value);
        
        printf("[%d] Updated all interfaces and system info\n", (int)time(NULL));
        
        counter++;
        sleep(2);
        
        if (counter % 5 == 0) {
            db_print_stats(db);
        }
    }
    
    db_close(db);
}

void run_client(const char *name, const char *subtree, int exact) {
    char path[MAX_KEY_LEN], value[MAX_VALUE_LEN];
    
    printf("=== Client '%s' (PID: %d) ===\n", name, getpid());
    printf("Opening database...\n");
    
    sleep(1);
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    printf("Subscribing to subtree: '%s' (%s)\n", 
           subtree, exact ? "exact match" : "subtree match");
    
    if (db_subscribe(db, subtree, exact) < 0) {
        fprintf(stderr, "Failed to subscribe\n");
        db_close(db);
        return;
    }
    
    printf("Listening for changes under '%s'...\n\n", subtree);
    
    while (1) {
        if (db_poll_notification(db, path, value)) {
            printf("[%s] %s = %s\n", name, path, value);
        }
        usleep(100000);
    }
    
    db_close(db);
}

void run_reader(const char *subtree) {
    char paths[100][MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    int count, i;
    
    printf("=== Reader (PID: %d) ===\n", getpid());
    printf("Reading subtree: '%s'\n\n", subtree);
    
    sleep(3);
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    count = db_list_subtree(db, subtree, paths, 100);
    printf("Found %d entries under '%s':\n", count, subtree);
    
    for (i = 0; i < count; i++) {
        if (db_get(db, paths[i], value, sizeof(value)) == 0) {
            printf("  %s = %s\n", paths[i], value);
        }
    }
    
    db_close(db);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s server                    - Run as server\n", argv[0]);
        printf("  %s eth0                      - Subscribe to interfaces/eth0 subtree\n", argv[0]);
        printf("  %s eth1                      - Subscribe to interfaces/eth1 subtree\n", argv[0]);
        printf("  %s interfaces                - Subscribe to all interfaces\n", argv[0]);
        printf("  %s system                    - Subscribe to system subtree\n", argv[0]);
        printf("  %s exact interfaces/eth0/status - Exact match subscription\n", argv[0]);
        printf("  %s read interfaces           - Read all entries under subtree\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (strcmp(argv[1], "eth0") == 0) {
        run_client("ETH0-MONITOR", "interfaces/eth0", 0);
    } else if (strcmp(argv[1], "eth1") == 0) {
        run_client("ETH1-MONITOR", "interfaces/eth1", 0);
    } else if (strcmp(argv[1], "interfaces") == 0) {
        run_client("ALL-INTERFACES", "interfaces", 0);
    } else if (strcmp(argv[1], "system") == 0) {
        run_client("SYSTEM-MONITOR", "system", 0);
    } else if (strcmp(argv[1], "exact") == 0 && argc > 2) {
        char name[64];
        snprintf(name, sizeof(name), "EXACT-%s", argv[2]);
        run_client(name, argv[2], 1);
    } else if (strcmp(argv[1], "read") == 0 && argc > 2) {
        run_reader(argv[2]);
    } else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}
