#include <stdio.h>
#include <string.h>

void foo(void) {
    printf("This is foo\n");
}

void bar(void) {
    printf("This is bar\n");
}

typedef void (*func_ptr_t)(void);

typedef struct {
    const char *name;
    func_ptr_t func;
} func_map_t;

func_map_t func_map[] = {
    {"foo", foo},
    {"bar", bar},
    {NULL, NULL} // Sentinel value to mark the end of the array
};

void call_func_by_name(const char *name) {
    for (int i = 0; func_map[i].name != NULL; ++i) {
        if (strcmp(func_map[i].name, name) == 0) {
            func_map[i].func();
            return;
        }
    }
    printf("Function not found: %s\n", name);
}

int main(void) {
    call_func_by_name("foo");
    call_func_by_name("bar");
    call_func_by_name("baz");

    return 0;
}

