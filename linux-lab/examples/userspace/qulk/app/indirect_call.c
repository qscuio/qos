#include <stdio.h>
#include "indirect_call.h"

int foo(int x) {
    printf("Called foo with %d\n", x);
    return x + 1;
}

int bar(int x) {
    printf("Called bar with %d\n", x);
    return x + 2;
}

int baz(int x) {
    printf("Called baz with %d\n", x);
    return x + 3;
}

int main_function(int (*func)(int), int x) {
    return INDIRECT_CALL_3(func, baz, bar, foo, x);
}

int main() {
    int (*function_ptr)(int);

    function_ptr = foo;
    printf("Result: %d\n", main_function(function_ptr, 5));

    function_ptr = bar;
    printf("Result: %d\n", main_function(function_ptr, 5));

    function_ptr = baz;
    printf("Result: %d\n", main_function(function_ptr, 5));

    function_ptr = NULL;
    printf("Result: %d\n", main_function(function_ptr, 5));

    return 0;
}
