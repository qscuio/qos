#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int n;
    int acc;
} Args;

typedef struct Frame {
    void (*func)(struct Frame*);
    Args args;
    struct Frame* next;
} Frame;

void factorial_step(Frame* frame);

void trampoline(Frame* frame) {
    while (frame != NULL) {
        Frame* next_frame = frame->next;
        frame->func(frame);
        frame = next_frame;
    }
}

void factorial_step(Frame* frame) {
    if (frame->args.n == 0) {
        // Final result is already in frame->args.acc
    } else {
        Frame* new_frame = malloc(sizeof(Frame));
        new_frame->func = factorial_step;
        new_frame->args.n = frame->args.n - 1;
        new_frame->args.acc = frame->args.acc * frame->args.n;
        new_frame->next = NULL;
        frame->next = new_frame;
    }
}

int factorial(int n) {
    Frame* initial_frame = malloc(sizeof(Frame));
    initial_frame->func = factorial_step;
    initial_frame->args.n = n;
    initial_frame->args.acc = 1;
    initial_frame->next = NULL;

    Frame* current_frame = initial_frame;
    while (current_frame->args.n > 0) {
        Frame* new_frame = malloc(sizeof(Frame));
        new_frame->func = factorial_step;
        new_frame->args.n = current_frame->args.n - 1;
        new_frame->args.acc = current_frame->args.acc * current_frame->args.n;
        new_frame->next = NULL;
        current_frame->next = new_frame;
        current_frame = new_frame;
    }

    int result = current_frame->args.acc;

    // Free all frames
    current_frame = initial_frame;
    while (current_frame != NULL) {
        Frame* next_frame = current_frame->next;
        free(current_frame);
        current_frame = next_frame;
    }

    return result;
}

int main() {
    int result = factorial(5);
    printf("Factorial of 5 is %d\n", result);
    return 0;
}

