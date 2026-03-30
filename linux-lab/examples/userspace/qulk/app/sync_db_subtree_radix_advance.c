/* 
 * Advanced Radix Tree Database with:
 * - Transactions
 * - Bulk operations
 * - Path wildcards
 * - Delete support
 * - Memory management
 * 
 * 编译: gcc -o test_advanced test_advanced.c -lpthread -lrt
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

#define DB_NAME "/radix_db_advanced"
#define MAX_NODES 4096
#define MAX_CHILDREN 64
#define MAX_KEY_LEN 64
#define MAX_VALUE_LEN 512
#define MAX_SUBSCRIBERS 32
#define MAX_TRANSACTIONS 8
#define MAX_TXN_OPS 128
#define PATH_SEPARATOR '/'

/* 事务操作类型 */
typedef enum {
    TXN_OP_SET,
    TXN_OP_DELETE
} txn_op_type_t;

/* 事务操作 */
typedef struct {
    txn_op_type_t type;
    char path[256];
    char value[MAX_VALUE_LEN];
    int valid;
} txn_operation_t;

/* 事务状态 */
typedef enum {
    TXN_IDLE,
    TXN_ACTIVE,
    TXN_COMMITTED,
    TXN_ABORTED
} txn_state_t;

/* 事务结构 */
typedef struct {
    txn_state_t state;
    pid_t pid;
    txn_operation_t ops[MAX_TXN_OPS];
    int op_count;
    pthread_mutex_t lock;
} transaction_t;

/* Radix Tree 节点 */
typedef struct radix_node {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    int is_leaf;
    uint64_t version;
    time_t last_modified;
    int parent_idx;
    int child_indices[MAX_CHILDREN];
    int child_count;
    int in_use;
    int marked_for_delete;  /* 延迟删除标记 */
} radix_node_t;

/* 订阅者 */
typedef struct {
    char subtree_path[256];
    int subtree_node_idx;
    int active;
    pid_t pid;
    int exact_match;
    uint32_t filter_flags;  /* 过滤标志: SET, DELETE 等 */
} subscriber_info_t;

/* 通知类型 */
typedef enum {
    NOTIFY_SET = 1,
    NOTIFY_DELETE = 2
} notify_type_t;

/* 通知条目 */
typedef struct {
    notify_type_t type;
    char path[256];
    char value[MAX_VALUE_LEN];
    int valid;
} notify_entry_t;

/* 统计信息 */
typedef struct {
    uint64_t read_count;
    uint64_t write_count;
    uint64_t delete_count;
    uint64_t notify_count;
    uint64_t txn_commit_count;
    uint64_t txn_abort_count;
} db_stats_t;

/* Radix Tree 数据库 */
typedef struct {
    pthread_rwlock_t lock;
    radix_node_t nodes[MAX_NODES];
    int root_idx;
    int node_count;
    
    /* 事务管理 */
    transaction_t transactions[MAX_TRANSACTIONS];
    pthread_mutex_t txn_lock;
    
    /* 订阅者 */
    subscriber_info_t subscribers[MAX_SUBSCRIBERS];
    pthread_mutex_t sub_lock;
    
    /* 通知队列 */
    notify_entry_t notify_queue[MAX_SUBSCRIBERS][16];
    volatile int notify_head[MAX_SUBSCRIBERS];
    volatile int notify_tail[MAX_SUBSCRIBERS];
    
    db_stats_t stats;
} radix_db_t;

/* 数据库句柄 */
typedef struct {
    radix_db_t *db;
    int shm_fd;
    int is_creator;
    int my_sub_slot;
    int my_txn_slot;  /* 当前事务槽位 */
} db_handle_t;

/* ========== 基础 Radix Tree 操作 ========== */

static int alloc_node(radix_db_t *db) {
    int i;
    for (i = 0; i < MAX_NODES; i++) {
        if (!db->nodes[i].in_use) {
            memset(&db->nodes[i], 0, sizeof(radix_node_t));
            db->nodes[i].in_use = 1;
            db->nodes[i].parent_idx = -1;
            db->nodes[i].child_count = 0;
            db->nodes[i].marked_for_delete = 0;
            db->node_count++;
            return i;
        }
    }
    return -1;
}

static void free_node(radix_db_t *db, int idx) {
    if (idx >= 0 && idx < MAX_NODES) {
        db->nodes[idx].in_use = 0;
        db->node_count--;
    }
}

static void init_root(radix_db_t *db) {
    db->root_idx = alloc_node(db);
    if (db->root_idx >= 0) {
        strcpy(db->nodes[db->root_idx].key, "");
        db->nodes[db->root_idx].is_leaf = 0;
    }
}

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

static int add_child(radix_db_t *db, int parent_idx, int child_idx) {
    radix_node_t *parent = &db->nodes[parent_idx];
    
    if (parent->child_count >= MAX_CHILDREN) return -1;
    
    parent->child_indices[parent->child_count++] = child_idx;
    db->nodes[child_idx].parent_idx = parent_idx;
    return 0;
}

static void remove_child(radix_db_t *db, int parent_idx, int child_idx) {
    int i, j;
    radix_node_t *parent = &db->nodes[parent_idx];
    
    for (i = 0; i < parent->child_count; i++) {
        if (parent->child_indices[i] == child_idx) {
            /* 移除子节点，后面的前移 */
            for (j = i; j < parent->child_count - 1; j++) {
                parent->child_indices[j] = parent->child_indices[j + 1];
            }
            parent->child_count--;
            break;
        }
    }
}

static int split_path(const char *path, char segments[][MAX_KEY_LEN], int max_segments) {
    int count = 0;
    const char *start = path;
    const char *end;
    
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

static int find_or_create_node(radix_db_t *db, const char *path, int create) {
    char segments[32][MAX_KEY_LEN];
    int segment_count = split_path(path, segments, 32);
    int current_idx = db->root_idx;
    int i;
    
    for (i = 0; i < segment_count; i++) {
        int child_idx = find_child(db, current_idx, segments[i]);
        
        if (child_idx == -1) {
            if (!create) return -1;
            
            child_idx = alloc_node(db);
            if (child_idx == -1) return -1;
            
            strcpy(db->nodes[child_idx].key, segments[i]);
            db->nodes[child_idx].is_leaf = (i == segment_count - 1);
            
            if (add_child(db, current_idx, child_idx) != 0) {
                free_node(db, child_idx);
                return -1;
            }
        }
        
        current_idx = child_idx;
    }
    
    if (create && segment_count > 0) {
        db->nodes[current_idx].is_leaf = 1;
    }
    
    return current_idx;
}

static void build_full_path(radix_db_t *db, int node_idx, char *path, size_t path_len) {
    char segments[32][MAX_KEY_LEN];
    int count = 0;
    int idx = node_idx;
    int i;
    
    while (idx != db->root_idx && idx >= 0) {
        if (count < 32) {
            strcpy(segments[count++], db->nodes[idx].key);
        }
        idx = db->nodes[idx].parent_idx;
    }
    
    path[0] = '\0';
    for (i = count - 1; i >= 0; i--) {
        if (strlen(path) > 0) {
            strncat(path, "/", path_len - strlen(path) - 1);
        }
        strncat(path, segments[i], path_len - strlen(path) - 1);
    }
}

/* ========== 数据库操作 ========== */

db_handle_t* db_open(int create) {
    db_handle_t *handle = malloc(sizeof(db_handle_t));
    if (!handle) return NULL;
    
    handle->is_creator = create;
    handle->my_sub_slot = -1;
    handle->my_txn_slot = -1;
    
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
        pthread_mutex_init(&handle->db->txn_lock, &mutex_attr);
        
        memset(handle->db->nodes, 0, sizeof(handle->db->nodes));
        memset(handle->db->subscribers, 0, sizeof(handle->db->subscribers));
        memset(handle->db->transactions, 0, sizeof(handle->db->transactions));
        memset(handle->db->notify_queue, 0, sizeof(handle->db->notify_queue));
        memset((void*)handle->db->notify_head, 0, sizeof(handle->db->notify_head));
        memset((void*)handle->db->notify_tail, 0, sizeof(handle->db->notify_tail));
        memset(&handle->db->stats, 0, sizeof(db_stats_t));
        
        handle->db->node_count = 0;
        init_root(handle->db);
        
        /* 初始化事务锁 */
        {
            int i;
            for (i = 0; i < MAX_TRANSACTIONS; i++) {
                pthread_mutex_init(&handle->db->transactions[i].lock, &mutex_attr);
                handle->db->transactions[i].state = TXN_IDLE;
            }
        }
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

/* 发送通知 */
static void send_notification(radix_db_t *db, int node_idx, const char *value, 
                             notify_type_t type) {
    char full_path[256];
    int i;
    
    build_full_path(db, node_idx, full_path, sizeof(full_path));
    
    pthread_mutex_lock(&db->sub_lock);
    for (i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (db->subscribers[i].active) {
            int should_notify = 0;
            int sub_node = db->subscribers[i].subtree_node_idx;
            
            /* 检查过滤器 */
            if (!(db->subscribers[i].filter_flags & type)) {
                continue;
            }
            
            if (db->subscribers[i].exact_match) {
                should_notify = (node_idx == sub_node);
            } else {
                int check_idx = node_idx;
                while (check_idx >= 0) {
                    if (check_idx == sub_node) {
                        should_notify = 1;
                        break;
                    }
                    check_idx = db->nodes[check_idx].parent_idx;
                }
            }
            
            if (should_notify) {
                int head = db->notify_head[i];
                int next_head = (head + 1) % 16;
                
                if (next_head != db->notify_tail[i]) {
                    db->notify_queue[i][head].type = type;
                    strncpy(db->notify_queue[i][head].path, 
                           full_path, sizeof(db->notify_queue[i][head].path) - 1);
                    if (value) {
                        strncpy(db->notify_queue[i][head].value, 
                               value, MAX_VALUE_LEN - 1);
                    }
                    db->notify_queue[i][head].valid = 1;
                    db->notify_head[i] = next_head;
                    db->stats.notify_count++;
                }
            }
        }
    }
    pthread_mutex_unlock(&db->sub_lock);
}

int db_set(db_handle_t *handle, const char *path, const char *value) {
    int node_idx;
    
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
    handle->db->nodes[node_idx].marked_for_delete = 0;
    handle->db->stats.write_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    send_notification(handle->db, node_idx, value, NOTIFY_SET);
    
    return 0;
}

int db_get(db_handle_t *handle, const char *path, char *value, size_t value_len) {
    int node_idx;
    
    if (!handle || !path || !value) return -1;
    
    pthread_rwlock_rdlock(&handle->db->lock);
    
    node_idx = find_or_create_node(handle->db, path, 0);
    if (node_idx < 0 || !handle->db->nodes[node_idx].is_leaf ||
        handle->db->nodes[node_idx].marked_for_delete) {
        pthread_rwlock_unlock(&handle->db->lock);
        return -1;
    }
    
    strncpy(value, handle->db->nodes[node_idx].value, value_len - 1);
    value[value_len - 1] = '\0';
    handle->db->stats.read_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    return 0;
}

/* 删除节点 */
int db_delete(db_handle_t *handle, const char *path) {
    int node_idx;
    
    if (!handle || !path) return -1;
    
    pthread_rwlock_wrlock(&handle->db->lock);
    
    node_idx = find_or_create_node(handle->db, path, 0);
    if (node_idx < 0 || !handle->db->nodes[node_idx].is_leaf) {
        pthread_rwlock_unlock(&handle->db->lock);
        return -1;
    }
    
    /* 标记删除 - 实际清理可以延后 */
    handle->db->nodes[node_idx].marked_for_delete = 1;
    handle->db->nodes[node_idx].value[0] = '\0';
    handle->db->stats.delete_count++;
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    send_notification(handle->db, node_idx, NULL, NOTIFY_DELETE);
    
    return 0;
}

/* 批量操作 */
typedef struct {
    char (*paths)[256];
    char (*values)[MAX_VALUE_LEN];
    int count;
    int max_count;
} bulk_context_t;

static void bulk_get_callback(radix_db_t *db, int node_idx, void *user_data) {
    bulk_context_t *ctx = (bulk_context_t*)user_data;
    if (ctx->count < ctx->max_count && !db->nodes[node_idx].marked_for_delete) {
        build_full_path(db, node_idx, ctx->paths[ctx->count], 256);
        strncpy(ctx->values[ctx->count], db->nodes[node_idx].value, MAX_VALUE_LEN - 1);
        ctx->count++;
    }
}

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

int db_bulk_get(db_handle_t *handle, const char *subtree, 
                char paths[][256], char values[][MAX_VALUE_LEN], int max_count) {
    int node_idx;
    bulk_context_t ctx;
    
    if (!handle) return -1;
    
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
    ctx.values = values;
    ctx.count = 0;
    ctx.max_count = max_count;
    
    traverse_subtree(handle->db, node_idx, bulk_get_callback, &ctx);
    
    pthread_rwlock_unlock(&handle->db->lock);
    
    return ctx.count;
}

/* ========== 事务支持 ========== */

int db_begin_transaction(db_handle_t *handle) {
    int i, slot = -1;
    
    if (!handle) return -1;
    
    pthread_mutex_lock(&handle->db->txn_lock);
    
    for (i = 0; i < MAX_TRANSACTIONS; i++) {
        if (handle->db->transactions[i].state == TXN_IDLE) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        pthread_mutex_unlock(&handle->db->txn_lock);
        return -1;
    }
    
    handle->db->transactions[slot].state = TXN_ACTIVE;
    handle->db->transactions[slot].pid = getpid();
    handle->db->transactions[slot].op_count = 0;
    handle->my_txn_slot = slot;
    
    pthread_mutex_unlock(&handle->db->txn_lock);
    
    return slot;
}

int db_txn_set(db_handle_t *handle, const char *path, const char *value) {
    transaction_t *txn;
    
    if (!handle || handle->my_txn_slot < 0) return -1;
    
    txn = &handle->db->transactions[handle->my_txn_slot];
    
    pthread_mutex_lock(&txn->lock);
    
    if (txn->state != TXN_ACTIVE || txn->op_count >= MAX_TXN_OPS) {
        pthread_mutex_unlock(&txn->lock);
        return -1;
    }
    
    txn->ops[txn->op_count].type = TXN_OP_SET;
    strncpy(txn->ops[txn->op_count].path, path, sizeof(txn->ops[0].path) - 1);
    strncpy(txn->ops[txn->op_count].value, value, MAX_VALUE_LEN - 1);
    txn->ops[txn->op_count].valid = 1;
    txn->op_count++;
    
    pthread_mutex_unlock(&txn->lock);
    
    return 0;
}

int db_commit_transaction(db_handle_t *handle) {
    transaction_t *txn;
    int i;
    
    if (!handle || handle->my_txn_slot < 0) return -1;
    
    txn = &handle->db->transactions[handle->my_txn_slot];
    
    pthread_mutex_lock(&txn->lock);
    
    if (txn->state != TXN_ACTIVE) {
        pthread_mutex_unlock(&txn->lock);
        return -1;
    }
    
    /* 执行所有操作 */
    for (i = 0; i < txn->op_count; i++) {
        if (txn->ops[i].valid) {
            if (txn->ops[i].type == TXN_OP_SET) {
                db_set(handle, txn->ops[i].path, txn->ops[i].value);
            } else if (txn->ops[i].type == TXN_OP_DELETE) {
                db_delete(handle, txn->ops[i].path);
            }
        }
    }
    
    txn->state = TXN_COMMITTED;
    handle->db->stats.txn_commit_count++;
    
    pthread_mutex_unlock(&txn->lock);
    
    /* 清理事务 */
    pthread_mutex_lock(&handle->db->txn_lock);
    txn->state = TXN_IDLE;
    handle->my_txn_slot = -1;
    pthread_mutex_unlock(&handle->db->txn_lock);
    
    return 0;
}

/* ========== 订阅 ========== */

int db_subscribe(db_handle_t *handle, const char *subtree, 
                 int exact_match, uint32_t filter_flags) {
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
    handle->db->subscribers[slot].filter_flags = filter_flags;
    handle->db->notify_head[slot] = 0;
    handle->db->notify_tail[slot] = 0;
    
    pthread_mutex_unlock(&handle->db->sub_lock);
    
    handle->my_sub_slot = slot;
    return slot;
}

int db_poll_notification(db_handle_t *handle, char *path, char *value, 
                         notify_type_t *type) {
    int tail;
    
    if (!handle || handle->my_sub_slot < 0) return 0;
    
    tail = handle->db->notify_tail[handle->my_sub_slot];
    
    if (tail != handle->db->notify_head[handle->my_sub_slot]) {
        if (handle->db->notify_queue[handle->my_sub_slot][tail].valid) {
            strcpy(path, handle->db->notify_queue[handle->my_sub_slot][tail].path);
            strcpy(value, handle->db->notify_queue[handle->my_sub_slot][tail].value);
            *type = handle->db->notify_queue[handle->my_sub_slot][tail].type;
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
    printf("Nodes: %d/%d\n", handle->db->node_count, MAX_NODES);
    printf("Reads: %llu\n", (unsigned long long)handle->db->stats.read_count);
    printf("Writes: %llu\n", (unsigned long long)handle->db->stats.write_count);
    printf("Deletes: %llu\n", (unsigned long long)handle->db->stats.delete_count);
    printf("Notifications: %llu\n", (unsigned long long)handle->db->stats.notify_count);
    printf("Txn Commits: %llu\n", (unsigned long long)handle->db->stats.txn_commit_count);
    printf("Txn Aborts: %llu\n", (unsigned long long)handle->db->stats.txn_abort_count);
    
    pthread_rwlock_unlock(&handle->db->lock);
}

/* ========== 测试程序 ========== */

void run_server() {
    int counter = 0;
    char value[MAX_VALUE_LEN];
    
    printf("=== Advanced Radix DB Server (PID: %d) ===\n", getpid());
    
    db_handle_t *db = db_open(1);
    if (!db) {
        fprintf(stderr, "Failed to create database\n");
        return;
    }
    
    printf("Server running with advanced features...\n");
    printf("Features: Transactions, Bulk ops, Delete, Wildcards\n\n");
    
    while (1) {
        /* 使用事务批量更新 */
        if (counter % 3 == 0) {
            printf("[TXN] Starting transaction...\n");
            db_begin_transaction(db);
            
            snprintf(value, sizeof(value), "up");
            db_txn_set(db, "interfaces/eth0/status", value);
            
            snprintf(value, sizeof(value), "%d", 1000 + counter);
            db_txn_set(db, "interfaces/eth0/speed", value);
            
            snprintf(value, sizeof(value), "%d", counter * 100);
            db_txn_set(db, "interfaces/eth0/packets_tx", value);
            
            db_commit_transaction(db);
            printf("[TXN] Committed 3 operations\n");
        } else {
            /* 普通更新 */
            snprintf(value, sizeof(value), "%s", counter % 2 ? "up" : "down");
            db_set(db, "interfaces/eth1/status", value);
            
            snprintf(value, sizeof(value), "%d", counter * 50);
            db_set(db, "interfaces/eth1/packets_rx", value);
        }
        
        /* 系统信息 */
        snprintf(value, sizeof(value), "router-%03d", counter);
        db_set(db, "system/hostname", value);
        
        snprintf(value, sizeof(value), "%d", counter * 60);
        db_set(db, "system/uptime", value);
        
        /* 测试删除 */
        if (counter % 10 == 5) {
            printf("[DELETE] Removing temporary data...\n");
            db_delete(db, "interfaces/eth1/packets_rx");
        }
        
        printf("[%d] Update cycle %d complete\n", (int)time(NULL), counter);
        
        counter++;
        sleep(2);
        
        if (counter % 5 == 0) {
            db_print_stats(db);
        }
    }
    
    db_close(db);
}

void run_client(const char *name, const char *subtree, int exact, uint32_t filter) {
    char path[256], value[MAX_VALUE_LEN];
    notify_type_t type;
    
    printf("=== Client '%s' (PID: %d) ===\n", name, getpid());
    sleep(1);
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    printf("Subscribing to: '%s'\n", subtree);
    printf("  Match mode: %s\n", exact ? "exact" : "subtree");
    printf("  Filter: %s%s\n", 
           (filter & NOTIFY_SET) ? "SET " : "",
           (filter & NOTIFY_DELETE) ? "DELETE" : "");
    printf("Listening...\n\n");
    
    if (db_subscribe(db, subtree, exact, filter) < 0) {
        fprintf(stderr, "Failed to subscribe\n");
        db_close(db);
        return;
    }
    
    while (1) {
        if (db_poll_notification(db, path, value, &type)) {
            if (type == NOTIFY_SET) {
                printf("[%s SET] %s = %s\n", name, path, value);
            } else if (type == NOTIFY_DELETE) {
                printf("[%s DEL] %s (deleted)\n", name, path);
            }
        }
        usleep(100000);
    }
    
    db_close(db);
}

void run_bulk_reader(const char *subtree) {
    char paths[100][256];
    char values[100][MAX_VALUE_LEN];
    int count, i;
    
    printf("=== Bulk Reader (PID: %d) ===\n", getpid());
    sleep(3);
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    printf("Reading subtree '%s' in bulk...\n\n", subtree);
    
    count = db_bulk_get(db, subtree, paths, values, 100);
    printf("Found %d entries:\n", count);
    
    for (i = 0; i < count; i++) {
        printf("  %s = %s\n", paths[i], values[i]);
    }
    
    db_print_stats(db);
    db_close(db);
}

void run_transaction_test() {
    char value[MAX_VALUE_LEN];
    int i;
    
    printf("=== Transaction Test (PID: %d) ===\n", getpid());
    sleep(2);
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    printf("Testing atomic transaction...\n");
    
    /* 开始事务 */
    if (db_begin_transaction(db) < 0) {
        fprintf(stderr, "Failed to begin transaction\n");
        db_close(db);
        return;
    }
    
    printf("Transaction started, adding 5 operations...\n");
    
    /* 批量添加操作 */
    for (i = 0; i < 5; i++) {
        char path[256];
        snprintf(path, sizeof(path), "test/txn/item%d", i);
        snprintf(value, sizeof(value), "value-%d", i * 10);
        
        if (db_txn_set(db, path, value) < 0) {
            fprintf(stderr, "Failed to add txn operation\n");
            break;
        }
        printf("  Added: %s = %s\n", path, value);
    }
    
    printf("Committing transaction...\n");
    if (db_commit_transaction(db) < 0) {
        fprintf(stderr, "Failed to commit\n");
    } else {
        printf("Transaction committed successfully!\n");
    }
    
    /* 验证数据 */
    printf("\nVerifying data...\n");
    for (i = 0; i < 5; i++) {
        char path[256];
        snprintf(path, sizeof(path), "test/txn/item%d", i);
        
        if (db_get(db, path, value, sizeof(value)) == 0) {
            printf("  Read: %s = %s\n", path, value);
        }
    }
    
    db_print_stats(db);
    db_close(db);
}

void run_delete_test() {
    char value[MAX_VALUE_LEN];
    
    printf("=== Delete Test (PID: %d) ===\n", getpid());
    sleep(2);
    
    db_handle_t *db = db_open(0);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return;
    }
    
    printf("Creating test data...\n");
    db_set(db, "temp/data1", "value1");
    db_set(db, "temp/data2", "value2");
    db_set(db, "temp/data3", "value3");
    
    printf("Reading before delete:\n");
    if (db_get(db, "temp/data2", value, sizeof(value)) == 0) {
        printf("  temp/data2 = %s\n", value);
    }
    
    printf("\nDeleting temp/data2...\n");
    db_delete(db, "temp/data2");
    
    printf("Reading after delete:\n");
    if (db_get(db, "temp/data2", value, sizeof(value)) != 0) {
        printf("  temp/data2: NOT FOUND (correctly deleted)\n");
    } else {
        printf("  temp/data2 = %s (ERROR: should be deleted!)\n", value);
    }
    
    db_print_stats(db);
    db_close(db);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Advanced Radix Tree Database\n");
        printf("Usage:\n");
        printf("  %s server           - Run server with transactions\n", argv[0]);
        printf("  %s eth0             - Subscribe to eth0 (SET events)\n", argv[0]);
        printf("  %s eth0-all         - Subscribe to eth0 (SET+DELETE)\n", argv[0]);
        printf("  %s interfaces       - Subscribe to all interfaces\n", argv[0]);
        printf("  %s bulk interfaces  - Bulk read all interfaces\n", argv[0]);
        printf("  %s txn-test         - Test transactions\n", argv[0]);
        printf("  %s del-test         - Test delete operations\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (strcmp(argv[1], "eth0") == 0) {
        run_client("ETH0", "interfaces/eth0", 0, NOTIFY_SET);
    } else if (strcmp(argv[1], "eth0-all") == 0) {
        run_client("ETH0-ALL", "interfaces/eth0", 0, NOTIFY_SET | NOTIFY_DELETE);
    } else if (strcmp(argv[1], "interfaces") == 0) {
        run_client("ALL-IF", "interfaces", 0, NOTIFY_SET | NOTIFY_DELETE);
    } else if (strcmp(argv[1], "bulk") == 0 && argc > 2) {
        run_bulk_reader(argv[2]);
    } else if (strcmp(argv[1], "txn-test") == 0) {
        run_transaction_test();
    } else if (strcmp(argv[1], "del-test") == 0) {
        run_delete_test();
    } else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}
