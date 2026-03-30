#define _GNU_SOURCE
#include <stdio.h>
#include <link.h>
#include <dlfcn.h>

// 声明 _r_debug 变量
extern struct r_debug _r_debug;

void print_link_map(struct link_map *map) {
    while (map) {
        printf("  Name: %s\n", map->l_name);
        printf("  Address: %p\n", (void *)map->l_addr);
        map = map->l_next;
    }
}

int main() {
    printf("r_debug:\n");
    printf("  r_version: %d\n", _r_debug.r_version);
    printf("  r_map: %p\n", (void *)_r_debug.r_map);
    printf("  r_brk: %p\n", (void *)_r_debug.r_brk);
    printf("  r_state: %d\n", _r_debug.r_state);
    printf("  r_ldbase: %p\n", (void *)_r_debug.r_ldbase);

    printf("\nLink map:\n");
    print_link_map(_r_debug.r_map);

    return 0;
}

