/* 
 * 多进程测试程序
 * 编译: gcc -o test_multi test_multi.c -lpthread -lrt
 * 运行: 
 *   终端1: ./test_multi server
 *   终端2: ./test_multi client1
 *   终端3: ./test_multi client2
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
#define DB_NAME "/global_db_test"
#define MAX_KEYS 1024
#define MAX_KEY_LEN 128
#define MAX_VALUE_LEN 512
#define MAX_SUBSCRIBERS 32

/* 数据库条目结构 */
typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    int valid;
    uint64_t version;
} db_entry_t;

/* 订阅者信息（进程本地） */
typedef struct {
    char pattern[MAX_KEY_LEN];
    int active;
    pid_t pid;  /* 订阅者进程ID */
} subscriber_info_t;

/* 全局数据库结构 */
typedef struct {
    pthread_rwlock_t lock;
    db_entry_t entries[MAX_KEYS];
    int entry_count;
    
    /* 订阅者管理 */
    subscriber_info_t subscribers[MAX_SUBSCRIBERS];
    pthread_mutex_t sub_lock;
    
    /* 通知标志位 */
    volatile int notify_flag[MAX_SUBSCRIBERS];
    char notify_key[MAX_SUBSCRIBERS][MAX_KEY_LEN];
    char notify_value[MAX_SUBSCRIBERS][MAX_VALUE_LEN];
    
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
    int my_sub_slot;  /* 本进程的订阅槽位 */
} db_handle_t;

/* ========== 核心数据库函数 ========== */

/* 创建或打开全局数据库 */
db_handle_t* db_open(int create) {
    db_handle_t *handle = malloc(sizeof(db_handle_t));
    if (!handle) return NULL;
    
    handle->is_creator = create;
    handle->my_sub_slot = -1;
    
    /* 打开或创建共享内存 */
    int flags = O_RDWR;
    if (create) {
        shm_unlink(DB_NAME);  /* 清理旧的 */
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
    
    /* 设置共享内存大小 */
    if (handle->is_creator) {
        if (ftruncate(handle->shm_fd, sizeof(global_db_t)) == -1) {
            perror("ftruncate");
            close(handle->shm_fd);
            free(handle);
            return NULL;
        }
    }
    
    /* 映射共享内存 */
    handle->db = mmap(NULL, sizeof(global_db_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED, 
                      handle->shm_fd, 0);
    
    if (handle->db == MAP_FAILED) {
        perror("mmap");
        close(handle->shm_fd);
        free(handle);
        return NULL;
    }
    
    /* 初始化数据库（仅创建者） */
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
        memset((void*)handle->db->notify_flag, 0, sizeof(handle->db->notify_flag));
        handle->db->entry_count = 0;
        handle->db->read_count = 0;
        handle->db->write_count = 0;
        handle->db->notify_count = 0;
    }
    
    return handle;
}

/* 关闭数据库 */
void db_close(db_handle_t *handle) {
    if (!handle) return;
    
    /* 清理订阅 */
    if (handle->my_sub_slot >= 0) {
        pthread_mutex_lock(&handle->db->sub_lock);
        handle->db->subscribers[handle->my_sub_slot].active = 0;
        pthread_mutex_unlock(&handle->db->sub_lock);
    }
    
    munmap(handle->db, sizeof(global_db_t));
    close(handle->shm_fd);
    
    /* 仅创建者删除共享内存 */
    if (handle->is_creator) {
        shm_unlink(DB_NAME);
    }
    
    free(handle);
}

/* 查找条目 */
static int find_entry(global_db_t *db, const char *key) {
    int i;
    for (i = 0; i < db->entry_count; i++) {
        if (db->entries[i].valid && 
            strcmp(db->entries[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

/* 写入数据 */
int db_set(db_handle_t *handle, const char *key, const char *value) {
    int i;
    int idx;
    
    if (!handle || !key || !value) return -1;
    
    pthread_rwlock_wrlock(&handle->db->lock);
    
    idx = find_entry(handle->db, key);
    
    if (idx == -1) {
        /* 新建条目 */
        if (handle->db->entry_count >= MAX_KEYS) {
            pthread_rwlock_unlock(&handle->db->lock);
            return -1;
        }
        idx = handle->db->entry_count++;
        strncpy(handle->db->entries[idx].key, key, MAX_KEY_LEN - 1);
        handle->db->entries[idx].valid = 1;
        handle->db->entries[idx].version = 0;
    }
    
    /* 更新值 */
    strncpy(handle->db->entries[idx].value, value, MAX_VALUE_LEN - 1);
    handle->db->entries[idx].version++;
    handle->db->write_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    /* 通知订阅者 */
    pthread_mutex_lock(&handle->db->sub_lock);
    for (i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (handle->db->subscribers[i].active) {
            /* 简单的前缀匹配 */
            if (strncmp(key, handle->db->subscribers[i].pattern,
                       strlen(handle->db->subscribers[i].pattern)) == 0) {
                /* 设置通知标志 */
                strncpy(handle->db->notify_key[i], key, MAX_KEY_LEN - 1);
                strncpy(handle->db->notify_value[i], value, MAX_VALUE_LEN - 1);
                handle->db->notify_flag[i] = 1;
                handle->db->notify_count++;
            }
        }
    }
    pthread_mutex_unlock(&handle->db->sub_lock);
    
    return 0;
}

/* 读取数据 */
int db_get(db_handle_t *handle, const char *key, char *value, size_t value_len) {
    int idx;
    
    if (!handle || !key || !value) return -1;
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    idx = find_entry(handle->db, key);
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

/* 订阅数据变更 */
int db_subscribe(db_handle_t *handle, const char *pattern) {
    int i;
    int slot = -1;
    
    if (!handle || !pattern) return -1;
    
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
    
    strncpy(handle->db->subscribers[slot].pattern, pattern, MAX_KEY_LEN - 1);
    handle->db->subscribers[slot].active = 1;
    handle->db->subscribers[slot].pid = getpid();
    handle->db->notify_flag[slot] = 0;
    
    pthread_mutex_unlock(&handle->db->sub_lock);
    
    handle->my_sub_slot = slot;
    return slot;
}

/* 轮询通知（订阅者调用） */
int db_poll_notification(db_handle_t *handle, char *key, char *value) {
    if (!handle || handle->my_sub_slot < 0) return 0;
    
    if (handle->db->notify_flag[handle->my_sub_slot]) {
        strcpy(key, handle->db->notify_key[handle->my_sub_slot]);
        strcpy(value, handle->db->notify_value[handle->my_sub_slot]);
        handle->db->notify_flag[handle->my_sub_slot] = 0;
        return 1;
    }
    
    return 0;
}

/* 打印统计信息 */
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
    printf("=== Server Process (PID: %d) ===\n", getpid());
    printf("Creating database...\n");
    
    db_handle_t *db = db_open(1);
    if (!db) {
        fprintf(stderr, "Failed to create database\n");
        return;
    }
    
    printf("Server running. Writing data every 2 seconds...\n");
    printf("Press Ctrl+C to stop.\n\n");
    
    while (1) {
        char key[64], value[64];
        
        /* 写入配置数据 */
        snprintf(key, sizeof(key), "config.counter");
        snprintf(value, sizeof(value), "%d", counter);
        db_set(db, key, value);
        printf("[%d] Set %s = %s\n", (int)time(NULL), key, value);
        
        /* 写入状态数据 */
        snprintf(key, sizeof(key), "status.timestamp");
        snprintf(value, sizeof(value), "%ld", time(NULL));
        db_set(db, key, value);
        
        counter++;
        sleep(2);
        
        if (counter % 5 == 0) {
            db_print_stats(db);
        }
    }
    
    db_close(db);
}

void run_client(const char *name, const char *pattern) {
    char key[MAX_KEY_LEN], value[MAX_VALUE_LEN];
    
    printf("=== Client '%s' (PID: %d) ===\n", name, getpid());
    printf("Opening database...\n");
    
    sleep(1);  /* 等待服务器创建数据库 */
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    /* 订阅数据变更 */
    printf("Subscribing to pattern: '%s'\n", pattern);
    if (db_subscribe(db, pattern) < 0) {
        fprintf(stderr, "Failed to subscribe\n");
        db_close(db);
        return;
    }
    
    printf("Listening for changes...\n\n");
    
    while (1) {
        /* 轮询通知 */
        if (db_poll_notification(db, key, value)) {
            printf("[%s NOTIFY] %s = %s\n", name, key, value);
        }
        
        usleep(100000);  /* 100ms 轮询间隔 */
    }
    
    db_close(db);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s server              - Run as server\n", argv[0]);
        printf("  %s client1             - Run as client1 (subscribe to 'config.')\n", argv[0]);
        printf("  %s client2             - Run as client2 (subscribe to 'status.')\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (strcmp(argv[1], "client1") == 0) {
        run_client("CLIENT1", "config.");
    } else if (strcmp(argv[1], "client2") == 0) {
        run_client("CLIENT2", "status.");
    } else {
        printf("Unknown mode: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}
