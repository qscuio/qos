#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

int main() {
    int        print = 0;
    ucontext_t ctx   = {0};
    getcontext(&ctx); // Loop start
    puts("Hello world");
    if (print > 5)
        exit(0);
    print++;
    setcontext(&ctx); // Loop end
    return EXIT_SUCCESS;
}
