#ifndef LOOM_INTERNAL_H
#define LOOM_INTERNAL_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Version 0.1.1. */
#define LOOM_VERSION_MAJOR 0
#define LOOM_VERSION_MINOR 1
#define LOOM_VERSION_PATCH 1

/* Opaque type for a thread pool. */
struct loom;

/* Configuration struct. */
typedef struct {
    /* Base-2 log for the task ring buffer, e.g. 8 => 256 slots.
     * A larger ring buffer takes more memory, but allows for a
     * larger backlog of tasks before the pool fills up and rejects
     * new tasks outright. */
    uint8_t ring_sz2;

    /* Max delay when for idle tasks' exponential back-off. */
    uint16_t max_delay;

    /* How many threads to start during initialization. Otherwise,
     * threads are started on demand, if new tasks are enqueued while
     * all others are busy. (Defaults to 0.) */
    uint16_t init_threads;

    /* The max number of threads to run for the thread pool.
     * Defaults to 8. */
    uint16_t max_threads;
} loom_config;

/* Callback to run, with an environment pointer. (A closure.) */
typedef void (loom_task_cb)(void *env);

/* Callback to clean up the environment if the thread pool is
 * shutting down & tasks are being canceled. */
typedef void (loom_cleanup_cb)(void *env);

/* A task to enqueue in the thread pool. *ENV is an arbitrary void pointer
 * that will be passed to the callbacks. */
typedef struct {
    loom_task_cb *task_cb;
    loom_cleanup_cb *cleanup_cb;
    void *env;
} loom_task;

/* Statistics from the currently running thread pool. */
typedef struct {
    uint16_t active_threads;
    uint16_t total_threads;
    size_t backlog_size;
} loom_info;

/* Initialize a thread pool in *L, according to the configuration in CFG. */
typedef enum {
    LOOM_INIT_RES_OK = 0,
    LOOM_INIT_RES_ERROR_NULL = -1,
    LOOM_INIT_RES_ERROR_BADARG = -2,
    LOOM_INIT_RES_ERROR_MEMORY = -3,
} loom_init_res;
loom_init_res loom_init(loom_config *cfg, struct loom **l);

/* Enqueue a task, which will be copied in to the thread pool by value.
 * Returns whether the task was successfully enqueued - it can fail if
 * the queue is full or if L or T are NULL.
 * 
 * If BACKPRESSURE is non-NULL, the current backlog size will be written
 * into it. This is a good way to push back against parts of the system
 * which are inundating the thread pool with tasks.
 * (*BACKPRESSURE / loom_queue_size(l) gives how full the queue is.) */
bool loom_enqueue(struct loom *l, loom_task *t, size_t *backpressure);

/* Get the size of the queue. */
size_t loom_queue_size(struct loom *l);

/* Get statistics from the currently running thread pool. */
bool loom_get_stats(struct loom *l, loom_info *info);

/* Send a shutdown notification to the thread pool. This may need to be
 * called multiple times, as threads will not cancel remaining tasks,
 * clean up, and terminate until they complete their current task, if any.
 * Returns whether all threads have shut down. (Idempotent.)*/
bool loom_shutdown(struct loom *l);

/* Free the thread pool and other internal resources. This will
 * internally call loom_shutdown until all threads have shut down. */
void loom_free(struct loom *l);

/* Defaults. */
#define DEF_RING_SZ2 8
#define DEF_MAX_DELAY 1000
#define DEF_MAX_THREADS 8

/* Max for log2(size) of task queue ring buffer.
 * The most significant bit of each cell is used as a mark. */
#define LOOM_MAX_RING_SZ2 ((8 * sizeof(size_t)) - 1)

/* Use mutexes instead of CAS?
 * This is mostly for benchmarking -- the lockless mode should be
 * significantly faster in most cases. */
#define LOOM_USE_LOCKING 0

typedef enum {
    LTS_INIT,                   /* initializing */
    LTS_ASLEEP,                 /* worker is asleep */
    LTS_ACTIVE,                 /* running a task */
    LTS_ALERT_SHUTDOWN,         /* worker should shut down */
    LTS_CLOSING,                /* worker is shutting down */
    LTS_JOINED,                 /* worker has been pthread_join'd */
    LTS_DEAD,                   /* pthread_create failed */
    LTS_RESPAWN,                /* slot reserved for respawn */
} thread_state;

typedef struct {
    pthread_t t;                /* thread handle */
    thread_state state;         /* task state machine state */
    int wr_fd;                  /* write end of alert pipe */
    int rd_fd;                  /* read end of alert pipe */
    struct loom *l;             /* pointer to thread pool */
} thread_info;

/* loom_task, with an added mark field. This is used mark whether a task is
 * ready to have the l->commit or l->done offsets advanced over it.
 *
 * . The mark bytes are all memset to 0xFF at init.
 * 
 * . The l->write offset can only advance across a cell if its mark
 *   has the most significant bit set.
 *    
 * . When a cell has been reserved for write (by atomically CAS-ing
 *   l->write to increment past it; during this time, only the producer
 *   thread reserving it can write to it), it is marked for commit by
 *   setting the mark to the write offset. Since the ring buffer wraps,
 *   this means the next time the same cell is used, the mark value will
 *   be based on the previous pass's write offset (l->write - l->size),
 *   which will no longer be valid.
 *
 * . After a write is committed, the producer thread does a CAS loop to
 *   advance l->commit over every marked cell. Since putting a task in
 *   or out of the queue is just a memcpy of an ltask to/from the
 *   caller's stack, it should never block for long, and have little
 *   variability in latency. It also doesn't matter which producer
 *   thread advances l->commit.
 *
 * . Similarly, a consumer atomically CASs l->read to reserve a cell
 *   for read, copies its task into its call stack, and then sets
 *   the cell mask to the negated read offset (~l->read). This means
 *   that it will always be distinct from the commit mark, distinct
 *   from the last spin around the ring buffer, and have the most
 *   significant bit set so that l->write knows it's free to advance
 *   over it.
 *
 * . Also, after the read is released, the consumer thread does a
 *   CAS loop to advance l->done over every marked cell. This behaves
 *   just like the CAS loop to advance l->commit above. */
typedef struct {
    loom_task_cb *task_cb;
    loom_cleanup_cb *cleanup_cb;
    void *env;
    size_t mark;
} ltask;

/* Offsets in ring buffer. Slot acquisition happens in the order
 * [Write, Commit, Read, Done]. For S(x) == loom->ring[x]:
 *          x >= W:  undefined
 *     C <= x <  W:  reserved for write
 *     R <= x <  C:  committed, available for read
 *     D <= x <  R:  being processed
 *          x <  D:  freed
 *
 *     W == C:  0 reserved
 *     R == C:  0 available
 *     D == R:  0 being read
 *
 * It's a ring buffer, so it wraps with (x & mask), and the
 * number of slots in useat a given time is:
 *     W - D
 *
 * Empty when W == D:
 *     [_, _, _, _,DW, _, _, _, ] {W:4, D:4} -- empty
 *
 * In use:
 *     (W - D)
 *     
 * Full when (reserved + 1) & mask == released & mask:
 *     [x, x, x, W, D, x, x, x, ] {W:3+8, D:4} -- full
 *
 *     D <= R <= C <= W
 */
typedef struct loom {
    #if LOOM_USE_LOCKING
    pthread_mutex_t lock;
    #endif
    /* Offsets. See block comment above. */
    size_t write;
    size_t commit;
    size_t read;
    size_t done;

    size_t size;                /* size of pool */
    size_t mask;                /* bitmask for pool offsets */
    uint16_t max_delay;         /* max poll(2) sleep for idle tasks */
    uint16_t cur_threads;       /* current live threads */
    uint16_t max_threads;       /* max # of threads to create */
    ltask *ring;                /* ring buffer */
    thread_info threads[];      /* thread metadata */
} loom;

typedef enum {
    ALERT_IDLE,                 /* no new tasks */
    ALERT_NEWTASK,              /* new task is available -- wake up */
    ALERT_ERROR,                /* unexpected read(2) failure */
    ALERT_SHUTDOWN,             /* threadpool is shutting down */
} alert_pipe_res;

#ifndef LOOM_LOG_LEVEL
#define LOOM_LOG_LEVEL 1
#endif

/* Log debugging info. */
#if LOOM_LOG_LEVEL > 0
#include <stdio.h>
#define LOG(LVL, ...)                                                  \
    do {                                                               \
        int lvl = LVL;                                                 \
        if ((LOOM_LOG_LEVEL) >= lvl) {                                 \
            printf(__VA_ARGS__);                                       \
        }                                                              \
    } while (0)
#else
#define LOG(LVL, ...)
#endif

#if LOOM_USE_LOCKING
#define LOCK(L)   if (0 != pthread_mutex_lock(&(L)->lock)) { assert(false); }
#define UNLOCK(L) if (0 != pthread_mutex_unlock(&(L)->lock)) { assert(false); }
#define CAS(PTR, OLD, NEW) (*PTR == (OLD) ? (*PTR = (NEW), 1) : 0)
#else
#define LOCK(L)   /* no-op */
#define UNLOCK(L) /* no-op */
#define CAS(PTR, OLD, NEW) (__sync_bool_compare_and_swap(PTR, OLD, NEW))
#endif

#endif
