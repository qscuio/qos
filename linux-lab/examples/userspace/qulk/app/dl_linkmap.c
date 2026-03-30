#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>

int main() {
    void *handle = dlopen("libm.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }

    struct link_map *linkMap;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &linkMap) == 0) {
        printf("Name: %s\n", linkMap->l_name);
        printf("Address: %p\n", (void *)linkMap->l_addr);
    } else {
        fprintf(stderr, "Error: %s\n", dlerror());
    }

    dlclose(handle);
    return 0;
}

