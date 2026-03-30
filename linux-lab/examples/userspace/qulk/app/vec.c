#include "vppinfra/vec.h"
#include "vppinfra/mem.h"
#include <stdio.h>

int main(int argc, char **argv) {

    int *v2    = NULL;
    int  value = 0;
    int *iter  = 0;

    clib_mem_init(0, 3ULL << 30);

    v2 = vec_new(int, 1024);

    vec_add1(v2, 10);
    vec_add1(v2, 30);
    vec_add1(v2, 40);
    vec_add1(v2, 20);
    vec_add1(v2, 100);
    vec_insert_init_empty(v2, 5, 5, 60);
    vec_pop2(v2, value);
    printf("vec_len %u, max_len %lu, bytes %lu\n", vec_len(v2), vec_max_len(v2),
           vec_bytes(v2));
    printf("vec_pop %d\n", value);
    vec_pop2(v2, value);
    printf("vec_pop %d\n", value);
    printf("vec_pop %d\n", vec_pop(v2));
    printf("vec_pop %d\n", vec_pop(v2));
    printf("vec_pop %d\n", vec_pop(v2));

    vec_set(v2, 11);
    printf("vec_pop %d\n", vec_pop(v2));
    vec_foreach(iter, v2) { printf("fv %d, ", *iter); };
    vec_foreach_index(value, v2) { printf("fi %d, ", value); };

    vec_foreach_backwards(iter, v2) { printf("fvb %d, ", *iter); };
    vec_foreach_index_backwards(value, v2) { printf("fib %d, ", value); };

#if 0
    vec_alloc(&v, 1024);
    char *v = NULL;
    printf("v size is %lu, v2 size is %lu\n", vec_mem_size(v),
           vec_mem_size(v2));
#endif
    printf("v size is %lu, v2 size is %lu, heap %p, align %lu\n",
           vec_mem_size(v2), vec_mem_size(v2), vec_get_heap(v2),
           vec_get_align(v2));

    return 0;
}
