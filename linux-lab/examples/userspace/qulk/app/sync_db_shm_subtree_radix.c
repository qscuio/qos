/* 
 * Radix Tree Database with Subtree Subscription
 * 编译: gcc -o test_radix test_radix.c -lpthread -lrt
 * 
 * 使用 Radix Tree (前缀树) 实现高效的层级数据库
 * 支持快速路径查找和子树遍历
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
#define DB_NAME "/global_db_radix"
#define MAX_NODES 4096           /* 最大节点数 */
#define MAX_CHILDREN 64          /* 每个节点最大子节点数 */
#define MAX_KEY_LEN 64           /* 单个路径段最大长度 */
#define MAX_VALUE_LEN 512
#define MAX_SUBSCRIBERS 32
#define PATH_SEPARATOR '/'

/* Radix Tree 节点结构 */
typedef struct radix_node {
    char key[MAX_KEY_LEN];       /* 路径段 (如 "eth0", "status") */
    char value[MAX_VALUE_LEN];   /* 叶子节点存储值 */
    
    int is_leaf;                 /* 是否是叶子节点 */
    uint64_t version;
    time_t last_modified;
    
    int parent_idx;              /* 父节点索引 */
    int child_indices[MAX_CHILDREN];  /* 子节点索引数组 */
    int child_count;
    
    int in_use;                  /* 节点是否在使用中 */
} radix_node_t;

/* 订阅者信息 */
typedef struct {
    char subtree_path[256];      /* 订阅的路径 */
    int subtree_node_idx;        /* 订阅节点的索引 */
    int active;
    pid_t pid;
    int exact_match;
} subscriber_info_t;

/* 通知队列 */
typedef struct {
    char path[256];
    char value[MAX_VALUE_LEN];
    int valid;
} notify_entry_t;

/* Radix Tree 数据库结构 */
typedef struct {
    pthread_rwlock_t lock;
    
    /* Radix Tree 节点池 */
    radix_node_t nodes[MAX_NODES];
    int root_idx;                /* 根节点索引 */
    int node_count;
    
    /* 订阅者管理 */
    subscriber_info_t subscribers[MAX_SUBSCRIBERS];
    pthread_mutex_t sub_lock;
    
    /* 通知队列 */
    notify_entry_t notify_queue[MAX_SUBSCRIBERS][16];
    volatile int notify_head[MAX_SUBSCRIBERS];
    volatile int notify_tail[MAX_SUBSCRIBERS];
    
    /* 统计信息 */
    uint64_t read_count;
    uint64_t write_count;
    uint64_t notify_count;
} radix_db_t;

/* 数据库句柄 */
typedef struct {
    radix_db_t *db;
    int shm_fd;
    int is_creator;
    int my_sub_slot;
} db_handle_t;

/* ========== Radix Tree 基础操作 ========== */

/* 分配新节点 */
static int alloc_node(radix_db_t *db) {
    int i;
    for (i = 0; i < MAX_NODES; i++) {
        if (!db->nodes[i].in_use) {
            memset(&db->nodes[i], 0, sizeof(radix_node_t));
            db->nodes[i].in_use = 1;
            db->nodes[i].parent_idx = -1;
            db->nodes[i].child_count = 0;
            db->node_count++;
            return i;
        }
    }
    return -1;
}

/* 初始化根节点 */
static void init_root(radix_db_t *db) {
    db->root_idx = alloc_node(db);
    if (db->root_idx >= 0) {
        strcpy(db->nodes[db->root_idx].key, "");
        db->nodes[db->root_idx].is_leaf = 0;
    }
}

/* 在父节点中查找子节点 */
static int find_child(radix_db_t *db, int parent_idx, const char *key) {
    int i;
    radix_node_t *parent = &db->nodes[parent_idx];
    
    for (i = 0; i < parent->child_count; i++) {
        int child_idx = parent->child_indices[i];
        if (strcmp(db->nodes[child_idx].key, key) == 0) {
            return child_idx;
        }
    }
    return -1;
}

/* 添加子节点 */
static int add_child(radix_db_t *db, int parent_idx, int child_idx) {
    radix_node_t *parent = &db->nodes[parent_idx];
    
    if (parent->child_count >= MAX_CHILDREN) {
        return -1;
    }
    
    parent->child_indices[parent->child_count++] = child_idx;
    db->nodes[child_idx].parent_idx = parent_idx;
    return 0;
}

/* 分割路径为段 */
static int split_path(const char *path, char segments[][MAX_KEY_LEN], int max_segments) {
    int count = 0;
    const char *start = path;
    const char *end;
    
    /* 跳过开头的斜杠 */
    while (*start == PATH_SEPARATOR) start++;
    
    while (*start && count < max_segments) {
        end = strchr(start, PATH_SEPARATOR);
        if (!end) {
            strncpy(segments[count], start, MAX_KEY_LEN - 1);
            segments[count][MAX_KEY_LEN - 1] = '\0';
            count++;
            break;
        }
        
        size_t len = end - start;
        if (len > 0 && len < MAX_KEY_LEN) {
            strncpy(segments[count], start, len);
            segments[count][len] = '\0';
            count++;
        }
        
        start = end + 1;
        while (*start == PATH_SEPARATOR) start++;
    }
    
    return count;
}

/* 查找或创建路径对应的节点 */
static int find_or_create_node(radix_db_t *db, const char *path, int create) {
    char segments[32][MAX_KEY_LEN];
    int segment_count = split_path(path, segments, 32);
    int current_idx = db->root_idx;
    int i;
    
    for (i = 0; i < segment_count; i++) {
        int child_idx = find_child(db, current_idx, segments[i]);
        
        if (child_idx == -1) {
            if (!create) return -1;
            
            /* 创建新节点 */
            child_idx = alloc_node(db);
            if (child_idx == -1) return -1;
            
            strcpy(db->nodes[child_idx].key, segments[i]);
            db->nodes[child_idx].is_leaf = (i == segment_count - 1);
            
            if (add_child(db, current_idx, child_idx) != 0) {
                db->nodes[child_idx].in_use = 0;
                return -1;
            }
        }
        
        current_idx = child_idx;
    }
    
    /* 如果是创建模式，确保最后的节点是叶子 */
    if (create && segment_count > 0) {
        db->nodes[current_idx].is_leaf = 1;
    }
    
    return current_idx;
}

/* 重建完整路径 */
static void build_full_path(radix_db_t *db, int node_idx, char *path, size_t path_len) {
    char segments[32][MAX_KEY_LEN];
    int count = 0;
    int idx = node_idx;
    int i;
    
    /* 从叶子向根收集路径段 */
    while (idx != db->root_idx && idx >= 0) {
        if (count < 32) {
            strcpy(segments[count++], db->nodes[idx].key);
        }
        idx = db->nodes[idx].parent_idx;
    }
    
    /* 反向构建路径 */
    path[0] = '\0';
    for (i = count - 1; i >= 0; i--) {
        if (strlen(path) > 0) {
            strncat(path, "/", path_len - strlen(path) - 1);
        }
        strncat(path, segments[i], path_len - strlen(path) - 1);
    }
}

/* 递归遍历子树 */
static void traverse_subtree(radix_db_t *db, int node_idx, 
                            void (*callback)(radix_db_t*, int, void*), 
                            void *user_data) {
    int i;
    radix_node_t *node = &db->nodes[node_idx];
    
    if (node->is_leaf) {
        callback(db, node_idx, user_data);
    }
    
    for (i = 0; i < node->child_count; i++) {
        traverse_subtree(db, node->child_indices[i], callback, user_data);
    }
}

/* ========== 数据库操作 ========== */

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
        if (ftruncate(handle->shm_fd, sizeof(radix_db_t)) == -1) {
            perror("ftruncate");
            close(handle->shm_fd);
            free(handle);
            return NULL;
        }
    }
    
    handle->db = mmap(NULL, sizeof(radix_db_t), 
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
        
        memset(handle->db->nodes, 0, sizeof(handle->db->nodes));
        memset(handle->db->subscribers, 0, sizeof(handle->db->subscribers));
        memset(handle->db->notify_queue, 0, sizeof(handle->db->notify_queue));
        memset((void*)handle->db->notify_head, 0, sizeof(handle->db->notify_head));
        memset((void*)handle->db->notify_tail, 0, sizeof(handle->db->notify_tail));
        
        handle->db->node_count = 0;
        handle->db->read_count = 0;
        handle->db->write_count = 0;
        handle->db->notify_count = 0;
        
        init_root(handle->db);
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
    
    munmap(handle->db, sizeof(radix_db_t));
    close(handle->shm_fd);
    
    if (handle->is_creator) {
        shm_unlink(DB_NAME);
    }
    
    free(handle);
}

int db_set(db_handle_t *handle, const char *path, const char *value) {
    int node_idx, i;
    char full_path[256];
    
    if (!handle || !path || !value) return -1;
    
    pthread_rwlock_wrlock(&handle->db->lock);
    
    node_idx = find_or_create_node(handle->db, path, 1);
    if (node_idx < 0) {
        pthread_rwlock_unlock(&handle->db->lock);
        return -1;
    }
    
    strncpy(handle->db->nodes[node_idx].value, value, MAX_VALUE_LEN - 1);
    handle->db->nodes[node_idx].version++;
    handle->db->nodes[node_idx].last_modified = time(NULL);
    handle->db->write_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    /* 通知订阅者 */
    build_full_path(handle->db, node_idx, full_path, sizeof(full_path));
    
    pthread_mutex_lock(&handle->db->sub_lock);
    for (i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (handle->db->subscribers[i].active) {
            int should_notify = 0;
            int sub_node = handle->db->subscribers[i].subtree_node_idx;
            
            if (handle->db->subscribers[i].exact_match) {
                should_notify = (node_idx == sub_node);
            } else {
                /* 检查 node_idx 是否在 sub_node 的子树中 */
                int check_idx = node_idx;
                while (check_idx >= 0) {
                    if (check_idx == sub_node) {
                        should_notify = 1;
                        break;
                    }
                    check_idx = handle->db->nodes[check_idx].parent_idx;
                }
            }
            
            if (should_notify) {
                int head = handle->db->notify_head[i];
                int next_head = (head + 1) % 16;
                
                if (next_head != handle->db->notify_tail[i]) {
                    strncpy(handle->db->notify_queue[i][head].path, 
                           full_path, sizeof(handle->db->notify_queue[i][head].path) - 1);
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

int db_get(db_handle_t *handle, const char *path, char *value, size_t value_len) {
    int node_idx;
    
    if (!handle || !path || !value) return -1;
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    node_idx = find_or_create_node(handle->db, path, 0);
    if (node_idx < 0 || !handle->db->nodes[node_idx].is_leaf) {
        pthread_rwlock_unlock(&handle->db->lock);
        return -1;
    }
    
    strncpy(value, handle->db->nodes[node_idx].value, value_len - 1);
    value[value_len - 1] = '\0';
    handle->db->read_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    return 0;
}

/* 列出子树的回调 */
typedef struct {
    char (*paths)[256];
    int count;
    int max_count;
} list_context_t;

static void list_callback(radix_db_t *db, int node_idx, void *user_data) {
    list_context_t *ctx = (list_context_t*)user_data;
    if (ctx->count < ctx->max_count) {
        build_full_path(db, node_idx, ctx->paths[ctx->count], 256);
        ctx->count++;
    }
}

int db_list_subtree(db_handle_t *handle, const char *subtree, 
                    char paths[][256], int max_paths) {
    int node_idx;
    list_context_t ctx;
    
    if (!handle || !paths) return -1;
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    if (strlen(subtree) == 0) {
        node_idx = handle->db->root_idx;
    } else {
        node_idx = find_or_create_node(handle->db, subtree, 0);
        if (node_idx < 0) {
            pthread_rwlock_unlock(&handle->db->lock);
            return 0;
        }
    }
    
    ctx.paths = paths;
    ctx.count = 0;
    ctx.max_count = max_paths;
    
    traverse_subtree(handle->db, node_idx, list_callback, &ctx);
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    return ctx.count;
}

int db_subscribe(db_handle_t *handle, const char *subtree, int exact_match) {
    int i, slot = -1, node_idx;
    
    if (!handle || !subtree) return -1;
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    if (strlen(subtree) == 0) {
        node_idx = handle->db->root_idx;
    } else {
        node_idx = find_or_create_node(handle->db, subtree, 0);
        if (node_idx < 0) {
            pthread_rwlock_unlock(&handle->db->lock);
            return -1;
        }
    }
    
    pthread_rwlock_unlock(&handle->db->lock);
    
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
    
    strncpy(handle->db->subscribers[slot].subtree_path, subtree, 
            sizeof(handle->db->subscribers[slot].subtree_path) - 1);
    handle->db->subscribers[slot].subtree_node_idx = node_idx;
    handle->db->subscribers[slot].active = 1;
    handle->db->subscribers[slot].pid = getpid();
    handle->db->subscribers[slot].exact_match = exact_match;
    handle->db->notify_head[slot] = 0;
    handle->db->notify_tail[slot] = 0;
    
    pthread_mutex_unlock(&handle->db->sub_lock);
    
    handle->my_sub_slot = slot;
    return slot;
}

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
    
    printf("\n=== Radix Tree Database Statistics ===\n");
    printf("Nodes: %d/%d\n", handle->db->node_count, MAX_NODES);
    printf("Reads: %llu\n", (unsigned long long)handle->db->read_count);
    printf("Writes: %llu\n", (unsigned long long)handle->db->write_count);
    printf("Notifications: %llu\n", (unsigned long long)handle->db->notify_count);
    
    pthread_rwlock_unlock(&handle->db->lock);
}

/* ========== 测试程序 (与之前相同) ========== */

void run_server() {
    int counter = 0;
    char value[MAX_VALUE_LEN];
    
    printf("=== Radix Tree Server (PID: %d) ===\n", getpid());
    
    db_handle_t *db = db_open(1);
    if (!db) {
        fprintf(stderr, "Failed to create database\n");
        return;
    }
    
    printf("Server running with Radix Tree backend...\n\n");
    
    while (1) {
        snprintf(value, sizeof(value), "%s", counter % 2 ? "up" : "down");
        db_set(db, "interfaces/eth0/status", value);
        
        snprintf(value, sizeof(value), "%d", 1000 + (counter % 100));
        db_set(db, "interfaces/eth0/speed", value);
        
        snprintf(value, sizeof(value), "%d", counter * 100);
        db_set(db, "interfaces/eth0/packets_tx", value);
        
        snprintf(value, sizeof(value), "up");
        db_set(db, "interfaces/eth1/status", value);
        
        snprintf(value, sizeof(value), "%d", counter * 50);
        db_set(db, "interfaces/eth1/packets_tx", value);
        
        snprintf(value, sizeof(value), "router-%03d", counter % 1000);
        db_set(db, "system/hostname", value);
        
        snprintf(value, sizeof(value), "%d", counter * 60);
        db_set(db, "system/uptime", value);
        
        printf("[%d] Updated (counter=%d)\n", (int)time(NULL), counter);
        
        counter++;
        sleep(2);
        
        if (counter % 5 == 0) {
            db_print_stats(db);
        }
    }
    
    db_close(db);
}

void run_client(const char *name, const char *subtree, int exact) {
    char path[256], value[MAX_VALUE_LEN];
    
    printf("=== Client '%s' (PID: %d) ===\n", name, getpid());
    sleep(1);
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    printf("Subscribing to: '%s' (%s match)\n", 
           subtree, exact ? "exact" : "subtree");
    
    if (db_subscribe(db, subtree, exact) < 0) {
        fprintf(stderr, "Failed to subscribe\n");
        db_close(db);
        return;
    }
    
    printf("Listening...\n\n");
    
    while (1) {
        if (db_poll_notification(db, path, value)) {
            printf("[%s] %s = %s\n", name, path, value);
        }
        usleep(100000);
    }
    
    db_close(db);
}

void run_reader(const char *subtree) {
    char paths[100][256];
    char value[MAX_VALUE_LEN];
    int count, i;
    
    printf("=== Reader (PID: %d) ===\n", getpid());
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
        printf("Radix Tree Database Demo\n");
        printf("Usage:\n");
        printf("  %s server\n", argv[0]);
        printf("  %s eth0\n", argv[0]);
        printf("  %s interfaces\n", argv[0]);
        printf("  %s read interfaces\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (strcmp(argv[1], "eth0") == 0) {
        run_client("ETH0", "interfaces/eth0", 0);
    } else if (strcmp(argv[1], "interfaces") == 0) {
        run_client("ALL-IF", "interfaces", 0);
    } else if (strcmp(argv[1], "read") == 0 && argc > 2) {
        run_reader(argv[2]);
    } else {
        printf("Unknown command\n");
        return 1;
    }
    
    return 0;
}
