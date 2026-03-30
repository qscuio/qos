#include <stdio.h>

//A shared library supports the ability to execute code upon load, by using the attribute((constructor)) decorator.
void __attribute__((constructor)) ctor(void) {
        printf("Library w loaded on dlopen()\n");
}

void __attribute__((destructor)) dtor(void) {
        printf("Library w unloaded on dlclose()\n");
}
