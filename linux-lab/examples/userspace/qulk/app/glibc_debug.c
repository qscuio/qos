#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <link.h>

// 声明有用的变量
extern struct r_debug _r_debug;

void print_link_map(struct link_map *map) {
    while (map) {
        printf("  Name: %s\n", map->l_name);
        printf("  Address: %p\n", (void *)map->l_addr);
        map = map->l_next;
    }
}

int main() {
    // 打印 _r_debug 结构的内容
    printf("_r_debug:\n");
    printf("  r_version: %d\n", _r_debug.r_version);
    printf("  r_map: %p\n", (void *)_r_debug.r_map);
    printf("  r_brk: %p\n", (void *)_r_debug.r_brk);
    printf("  r_state: %d\n", _r_debug.r_state);
    printf("  r_ldbase: %p\n", (void *)_r_debug.r_ldbase);

    printf("\nLink map (_r_debug):\n");
    print_link_map(_r_debug.r_map);

    // 使用 dlsym 获取 _dl_loaded, _dl_rtld_map 和 _dl_starting_up 的地址
    void *handle = dlopen("ld-linux-x86-64.so.2", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to open ld-linux.so.2: %s\n", dlerror());
        return 1;
    }

    struct link_map *dl_loaded = (struct link_map *)dlsym(handle, "_dl_loaded");
    if (dl_loaded) {
        printf("\n_dl_loaded:\n");
        print_link_map(dl_loaded);
    } else {
        printf("\nFailed to find _dl_loaded: %s\n", dlerror());
    }

    struct link_map *dl_rtld_map = (struct link_map *)dlsym(handle, "_dl_rtld_map");
    if (dl_rtld_map) {
        printf("\n_dl_rtld_map:\n");
        printf("  Name: %s\n", dl_rtld_map->l_name);
        printf("  Address: %p\n", (void *)dl_rtld_map->l_addr);
    } else {
        printf("\nFailed to find _dl_rtld_map: %s\n", dlerror());
    }

    int *dl_starting_up = (int *)dlsym(handle, "_dl_starting_up");
    if (dl_starting_up) {
        printf("\n_dl_starting_up: %d\n", *dl_starting_up);
    } else {
        printf("\nFailed to find _dl_starting_up: %s\n", dlerror());
    }

    dlclose(handle);

    return 0;
}

