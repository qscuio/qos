#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

#define MAX_TIMERS 100

// Timer callback function type
typedef void (*timer_callback_t)(void *);

// Timer structure
typedef struct {
    long long expiration_time;  // Expiration time in milliseconds
    timer_callback_t callback;  // Function to call when the timer expires
    void *callback_arg;         // Argument passed to the callback function
} _timer_t;

// Heap structure
typedef struct {
    _timer_t timers[MAX_TIMERS];  // Array to store the timers
    int size;                    // Current number of timers in the heap
} timer_heap_t;

// Get the current time in milliseconds
long long current_time_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

// Swap two timers in the heap
void swap(_timer_t *a, _timer_t *b) {
    _timer_t temp = *a;
    *a = *b;
    *b = temp;
}

// Insert a new timer into the heap
void heap_insert(timer_heap_t *heap, _timer_t timer) {
    if (heap->size >= MAX_TIMERS) {
        printf("Heap is full!\n");
        return;
    }

    // Insert the timer at the end of the heap
    int i = heap->size++;
    heap->timers[i] = timer;

    // Bubble up to maintain heap property
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap->timers[parent].expiration_time <= heap->timers[i].expiration_time) {
            break;
        }
        swap(&heap->timers[i], &heap->timers[parent]);
        i = parent;
    }
}

// Remove the root (the earliest timer) from the heap
void heap_remove_root(timer_heap_t *heap) {
    if (heap->size == 0) {
        printf("Heap is empty!\n");
        return;
    }

    // Replace the root with the last element
    heap->timers[0] = heap->timers[--heap->size];

    // Bubble down to maintain heap property
    int i = 0;
    while (true) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < heap->size && heap->timers[left].expiration_time < heap->timers[smallest].expiration_time) {
            smallest = left;
        }
        if (right < heap->size && heap->timers[right].expiration_time < heap->timers[smallest].expiration_time) {
            smallest = right;
        }

        if (smallest == i) {
            break;
        }

        swap(&heap->timers[i], &heap->timers[smallest]);
        i = smallest;
    }
}

// Peek the root (earliest timer) of the heap without removing it
_timer_t *heap_peek(timer_heap_t *heap) {
    if (heap->size == 0) {
        return NULL;
    }
    return &heap->timers[0];
}

// Initialize the heap
void heap_init(timer_heap_t *heap) {
    heap->size = 0;
}

// Timer callback function to test the timer library
void test_callback(void *arg) {
    printf("Timer expired: %s\n", (char *)arg);
}

// Add a new timer
void add_timer(timer_heap_t *heap, long long delay_ms, timer_callback_t callback, void *arg) {
    long long expiration_time = current_time_millis() + delay_ms;
    _timer_t new_timer = {expiration_time, callback, arg};
    heap_insert(heap, new_timer);
}

// Main loop to process timers
void process_timers(timer_heap_t *heap) {
    while (1) {
        // Get the current time
        long long now = current_time_millis();

        // Check if there's a timer that has expired
        _timer_t *timer = heap_peek(heap);
        if (timer && timer->expiration_time <= now) {
            // Call the timer's callback
            timer->callback(timer->callback_arg);

            // Remove the expired timer from the heap
            heap_remove_root(heap);
        } else if (timer) {
            // Sleep for a short period if no timer is ready
            usleep(1000);  // Sleep for 1 millisecond
        }
    }
}

int main() {
    // Initialize the heap
    timer_heap_t timer_heap;
    heap_init(&timer_heap);

    // Add some test timers
    add_timer(&timer_heap, 2000, test_callback, "Timer 1 (2 seconds)");
    add_timer(&timer_heap, 1000, test_callback, "Timer 2 (1 second)");
    add_timer(&timer_heap, 5000, test_callback, "Timer 3 (5 seconds)");
    add_timer(&timer_heap, 3000, test_callback, "Timer 4 (3 seconds)");
    add_timer(&timer_heap, 3000, test_callback, "Timer 5 (3 seconds)");
    add_timer(&timer_heap, 3000, test_callback, "Timer 6 (3 seconds)");
    add_timer(&timer_heap, 3000, test_callback, "Timer 7 (3 seconds)");
    add_timer(&timer_heap, 3000, test_callback, "Timer 8 (3 seconds)");
    add_timer(&timer_heap, 3000, test_callback, "Timer 9 (3 seconds)");
    add_timer(&timer_heap, 6000, test_callback, "Timer 10 (6 seconds)");
    add_timer(&timer_heap, 7000, test_callback, "Timer 11 (7 seconds)");

    // Main loop to process timers
    process_timers(&timer_heap);

    return 0;
}

