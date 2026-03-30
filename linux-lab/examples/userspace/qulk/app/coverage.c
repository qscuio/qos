#include "coverage.h"

COVERAGE_DEFINE(foo);
COVERAGE_DEFINE(bar);

int fn_foo(void) {
    COVERAGE_INC(foo);
    return 0;
}
int fn_bar(void) {
    COVERAGE_INC(bar);
    return 0;
}

int main(int argc, char **argv) {
    int i = 0;

    coverage_init();
    for (i = 0; i < 1000; i++) {
        fn_foo();
        fn_bar();
        coverage_run();
        coverage_log();
    }
}
