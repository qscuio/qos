#include "aco.h"
#include <stdio.h>

// this header would override the default C `assert`;
// you may refer the "API : MACROS" part for more details.
#include "aco_assert_override.h"

void foo(int ct) {
    printf("co: %p: yield to main_co: %d\n", aco_get_co(),
           *((int *) (aco_get_arg())));
    aco_yield();
    *((int *) (aco_get_arg())) = ct + 1;
}

void co_fp0() {
    printf("co: %p: entry: %d\n", aco_get_co(), *((int *) (aco_get_arg())));
    int ct = 0;
    while (ct < 6) {
        foo(ct);
        ct++;
    }
    printf("co: %p:  exit to main_co: %d\n", aco_get_co(),
           *((int *) (aco_get_arg())));
    aco_exit();
}

int main() {
    aco_thread_init(NULL);

    aco_t *            main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    aco_share_stack_t *sstk    = aco_share_stack_new(0);

    int    co_ct_arg_point_to_me = 0;
    aco_t *co = aco_create(main_co, sstk, 0, co_fp0, &co_ct_arg_point_to_me);

    int ct = 0;
    while (ct < 6) {
        assert(co->is_end == 0);
        printf("main_co: yield to co: %p: %d\n", co, ct);
        aco_resume(co);
        assert(co_ct_arg_point_to_me == ct);
        ct++;
    }
    printf("main_co: yield to co: %p: %d\n", co, ct);
    aco_resume(co);
    assert(co_ct_arg_point_to_me == ct);
    assert(co->is_end);

    printf("main_co: destroy and exit\n");
    aco_destroy(co);
    co = NULL;
    aco_share_stack_destroy(sstk);
    sstk = NULL;
    aco_destroy(main_co);
    main_co = NULL;

    return 0;
}
