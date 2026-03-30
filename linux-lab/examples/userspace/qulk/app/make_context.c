#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
void assign(uint32_t *var, uint32_t val) { 
    *var = val; 
}

int main( ) {
    uint32_t var = 0;
    ucontext_t ctx = {0}, back = {0};
    getcontext(&ctx);
    ctx.uc_stack.ss_sp = calloc(1, MINSIGSTKSZ);
    ctx.uc_stack.ss_size = MINSIGSTKSZ;
    ctx.uc_stack.ss_flags = 0;
    ctx.uc_link = &back; // Will get back to main as `swapcontext` call will populate `back` with current context
    // ctx.uc_link = 0;  // Will exit directly after `swapcontext` call
    makecontext(&ctx, (void (*)())assign, 2, &var, 100);
    swapcontext(&back, &ctx);    // Calling `assign` by switching context
    printf("var = %d\n", var);
    return EXIT_SUCCESS;
}
