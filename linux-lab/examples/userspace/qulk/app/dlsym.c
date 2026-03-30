#include <dlfcn.h>
#include <stdio.h>

int main() {
    void *handle = dlopen("libm.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }
    double (*cosine)(double) = dlsym(handle, "cos");
    if (!cosine) {
        fprintf(stderr, "Error: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    printf("cos(2.0) = %f\n", cosine(2.0));
    dlclose(handle);
    return 0;
}

