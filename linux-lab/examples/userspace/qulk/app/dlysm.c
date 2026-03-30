#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main() {
    // Load the C standard library (libc)
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading libc: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    // Clear any existing errors
    dlerror();

    // Get the address of printf
    typedef int (*printf_t)(const char *, ...);
    printf_t my_printf = (printf_t) dlsym(handle, "printf");

    // Check for errors
    const char *error = dlerror();
    if (error) {
        fprintf(stderr, "Error finding printf: %s\n", error);
        dlclose(handle);
        exit(EXIT_FAILURE);
    }

    // Use the resolved printf function
    my_printf("Hello, World!\n");

    // Close the library
    dlclose(handle);

    return 0;
}

