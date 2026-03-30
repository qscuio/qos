#include <time.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define MAX_EVENTS 10
#define TIMER_HEAP_INIT_CAPACITY 16

typedef void (*io_cb)(int, uint32_t, void*);
typedef void (*timer_cb)(void*);
typedef void (*signal_cb)(int, void*);
typedef void (*user_cb)(void*);

struct timer_event {
    struct timespec expiry;
    timer_cb callback;
    void* data;
};

struct timer_heap {
    struct timer_event** heap;
    size_t size;
    size_t capacity;
};

struct event_loop {
    int epoll_fd;
    int signal_fd;
    int user_event_pipe[2];
    pthread_mutex_t mutex;
    struct timer_heap timers;
    sigset_t signal_mask;
    user_cb user_event_cb;
    void* user_data;
};


struct io_event {
    io_cb callback;
    void* data;
};


/* Min-heap implementation */
static void timer_heap_init(struct timer_heap* heap) {
    heap->capacity = TIMER_HEAP_INIT_CAPACITY;
    heap->size = 0;
    heap->heap = malloc(sizeof(struct timer_event*) * heap->capacity);
}

static void timer_heap_swap(struct timer_heap* heap, size_t i, size_t j) {
    struct timer_event* tmp = heap->heap[i];
    heap->heap[i] = heap->heap[j];
    heap->heap[j] = tmp;
}

static int timer_compare(struct timer_event* a, struct timer_event* b) {
    if (a->expiry.tv_sec != b->expiry.tv_sec)
        return a->expiry.tv_sec < b->expiry.tv_sec ? -1 : 1;
    return a->expiry.tv_nsec < b->expiry.tv_nsec ? -1 : 1;
}

static void heapify_up(struct timer_heap* heap, size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (timer_compare(heap->heap[index], heap->heap[parent]) < 0) {
            timer_heap_swap(heap, index, parent);
            index = parent;
        } else break;
    }
}

static void heapify_down(struct timer_heap* heap, size_t index) {
    while (1) {
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t smallest = index;

        if (left < heap->size && 
            timer_compare(heap->heap[left], heap->heap[smallest]) < 0)
            smallest = left;
        if (right < heap->size && 
            timer_compare(heap->heap[right], heap->heap[smallest]) < 0)
            smallest = right;
        
        if (smallest != index) {
            timer_heap_swap(heap, index, smallest);
            index = smallest;
        } else break;
    }
}

static void timer_heap_push(struct timer_heap* heap, struct timer_event* event) {
    if (heap->size >= heap->capacity) {
        heap->capacity *= 2;
        heap->heap = realloc(heap->heap, sizeof(struct timer_event*) * heap->capacity);
    }
    heap->heap[heap->size++] = event;
    heapify_up(heap, heap->size - 1);
}

static struct timer_event* timer_heap_pop(struct timer_heap* heap) {
    if (heap->size == 0) return NULL;
    struct timer_event* top = heap->heap[0];
    heap->heap[0] = heap->heap[--heap->size];
    heapify_down(heap, 0);
    return top;
}

int event_loop_add_timer(struct event_loop* loop, unsigned long ms, timer_cb cb, void* data) {
    struct timer_event* te = malloc(sizeof(*te));
    clock_gettime(CLOCK_MONOTONIC, &te->expiry);
    te->expiry.tv_sec += ms / 1000;
    te->expiry.tv_nsec += (ms % 1000) * 1000000;
    if (te->expiry.tv_nsec >= 1000000000) {
        te->expiry.tv_sec += 1;
        te->expiry.tv_nsec -= 1000000000;
    }
    te->callback = cb;
    te->data = data;

    pthread_mutex_lock(&loop->mutex);
    timer_heap_push(&loop->timers, te);
    pthread_mutex_unlock(&loop->mutex);
    return 0;
}

int event_loop_add_signal(struct event_loop* loop, int sig, signal_cb cb, void* data) {
    sigaddset(&loop->signal_mask, sig);
    sigprocmask(SIG_BLOCK, &loop->signal_mask, NULL);
    
    if (loop->signal_fd == 0) {
        loop->signal_fd = signalfd(-1, &loop->signal_mask, SFD_NONBLOCK);
        struct io_event* sigev = malloc(sizeof(*sigev));
        sigev->callback = (io_cb)cb;
        sigev->data = data;
        
        struct epoll_event ev = {.events = EPOLLIN, .data.ptr = sigev};
        epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, loop->signal_fd, &ev);
    }
    return 0;
}

void event_loop_set_user(struct event_loop* loop, user_cb cb, void* data) {
    loop->user_event_cb = cb;
    loop->user_data = data;
    struct epoll_event ev = {.events = EPOLLIN};
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, loop->user_event_pipe[0], &ev);
}

void event_loop_trigger_user(struct event_loop* loop) {
    write(loop->user_event_pipe[1], "!", 1);
}


struct event_loop* event_loop_create() {
    struct event_loop* loop = calloc(1, sizeof(struct event_loop));
    loop->epoll_fd = epoll_create1(0);
    sigemptyset(&loop->signal_mask);

    /* Initialize user event pipe with non-blocking flag */
    if (pipe(loop->user_event_pipe) == -1) {
        perror("pipe");
        free(loop);
        return NULL;
    }
    fcntl(loop->user_event_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(loop->user_event_pipe[1], F_SETFL, O_NONBLOCK);

    timer_heap_init(&loop->timers);
    pthread_mutex_init(&loop->mutex, NULL);
    return loop;
}

int event_loop_add_io(struct event_loop* loop, int fd, uint32_t events, io_cb cb, void* data) {
    struct io_event* ioev = malloc(sizeof(*ioev));
    ioev->callback = cb;
    ioev->data = data;

    struct epoll_event ev = {
        .events = events,
        .data.ptr = ioev
    };
    return epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void event_loop_run(struct event_loop* loop) {
    struct epoll_event events[MAX_EVENTS];
    
    while (1) {
        /* Calculate next timeout */
        int timeout = -1;
        pthread_mutex_lock(&loop->mutex);
        if (loop->timers.size > 0) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            struct timer_event* next = loop->timers.heap[0];
            timeout = (next->expiry.tv_sec - now.tv_sec) * 1000 +
                     (next->expiry.tv_nsec - now.tv_nsec) / 1000000;
            if (timeout < 0) timeout = 0;
        }
        pthread_mutex_unlock(&loop->mutex);

        /* Wait for events */
        int n = epoll_wait(loop->epoll_fd, events, MAX_EVENTS, timeout);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == loop->signal_fd) {
                struct signalfd_siginfo info;
                read(loop->signal_fd, &info, sizeof(info));
                signal_cb cb = events[i].data.ptr;
                if (cb) cb(info.ssi_signo, NULL);
            }
            else if (events[i].data.fd == loop->user_event_pipe[0]) {
                char buf[256];
                read(loop->user_event_pipe[0], buf, sizeof(buf));
                if (loop->user_event_cb) loop->user_event_cb(loop->user_data);
            }
            else {
                io_cb cb = events[i].data.ptr;
                if (cb) cb(events[i].data.fd, events[i].events, NULL);
            }
        }

        /* Process expired timers */
        pthread_mutex_lock(&loop->mutex);
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        while (loop->timers.size > 0) {
            struct timer_event* next = loop->timers.heap[0];
            if (timer_compare(next, &(struct timer_event){.expiry = now}) <= 0) {
                timer_heap_pop(&loop->timers);
                next->callback(next->data);
                free(next);
            } else break;
        }
        pthread_mutex_unlock(&loop->mutex);
    }
}

/* Example usage */
void timer_handler(void* data) {
    printf("Timer fired: %s\n", (char*)data);
}

void io_handler(int fd, uint32_t events, void* data) {
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) printf("IO event: %.*s\n", (int)n, buf);
}

void signal_handler(int sig, void* data) {
    printf("Received signal: %d\n", sig);
}

void user_handler(void* data) {
    printf("User event: %s\n", (char*)data);
}

int main() {
    struct event_loop* loop = event_loop_create();
    
    event_loop_add_timer(loop, 2000, timer_handler, "2sec timer");
    event_loop_add_io(loop, STDIN_FILENO, EPOLLIN, io_handler, NULL);
    event_loop_add_signal(loop, SIGINT, signal_handler, NULL);
    event_loop_set_user(loop, user_handler, "custom data");
    
    /* Trigger user event from another thread */
    pthread_t thread;
    pthread_create(&thread, NULL, (void*)event_loop_trigger_user, loop);
    
    event_loop_run(loop);
    return 0;
}
