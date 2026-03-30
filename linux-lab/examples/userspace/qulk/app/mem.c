#include "vppinfra/mem.h"
#include "vppinfra/format.h"
#include "vppinfra/vec.h"

#include <stdio.h>

int main(int argc, char **argv) {
    clib_mem_init(0, 3ULL << 30);
    clib_mem_trace(1);

    clib_mem_alloc(100);
    clib_mem_alloc(100);
    clib_mem_alloc(100);
    clib_mem_alloc(100);

    printf("traced %d, heap %p\n", clib_mem_is_traced(),
           clib_mem_get_per_cpu_heap());

    fformat(stderr, "%U\n", format_clib_mem_usage, /* verbose */ 1);
}
