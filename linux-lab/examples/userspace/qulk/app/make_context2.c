#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE 1024*64

ucontext_t context1, context2, context_main;

void func1() {
    printf("Entering func1\n");
    
    // Switch to context2
    swapcontext(&context1, &context2);

    printf("Exiting func1\n");
}

void func2() {
    printf("Entering func2\n");

    // Switch back to context1
    swapcontext(&context2, &context1);

    printf("Exiting func2\n");
}

int main() {
    // Allocate stacks for the contexts
    char stack1[STACK_SIZE];
    char stack2[STACK_SIZE];

    // Initialize context1
    getcontext(&context1);
    context1.uc_link = &context_main;  // When context1 returns, switch to context_main
    context1.uc_stack.ss_sp = stack1;
    context1.uc_stack.ss_size = sizeof(stack1);
    makecontext(&context1, func1, 0);

    // Initialize context2
    getcontext(&context2);
    context2.uc_link = &context_main;  // When context2 returns, switch to context_main
    context2.uc_stack.ss_sp = stack2;
    context2.uc_stack.ss_size = sizeof(stack2);
    makecontext(&context2, func2, 0);

    printf("Switching to context1\n");
    
    // Switch to context1
    swapcontext(&context_main, &context1);

    printf("Back in main context\n");

    return 0;
}

