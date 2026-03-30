#include <stdio.h>
#include <stdlib.h>
#include "foo.h"
#include "plthook.h"

static int (*org_bar)(int a, int b);
static int (*org_baa)(int a, int b, int c);

void print_plt_entries(const char *filename)
{
    plthook_t *plthook;
    unsigned int pos = 0; /* This must be initialized with zero. */
    const char *name;
    void **addr;

    if (plthook_open(&plthook, filename) != 0) {
        printf("plthook_open error: %s\n", plthook_error());
        return;
    }
    while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
        printf("%p(%p) %s\n", addr, *addr, name);
    }
    plthook_close(plthook);
    return;
}

/* This function is called instead of recv() called by libfoo.so.1  */
static int bar_hook(int a, int b)
{
    printf("Call %s\n", __func__);
    return (*org_bar)(++a, ++b);
}

static int baa_hook(int a, int b, int c)
{
    printf("Call %s\n", __func__);
    return (*org_baa)(++a, ++b, ++c);
}

int install_hook_function(void)
{
    plthook_t *plthook;
    
    if (plthook_open(&plthook, "./libfoo.so") != 0) {
        printf("plthook_open error: %s\n", plthook_error());
        return -1;
    }

    if (plthook_replace(plthook, "bar", (void *)bar_hook, (void *)&org_bar) != 0) {
        printf("plthook_replace error: %s\n", plthook_error());
        plthook_close(plthook);
        return -1;
    }

    if (plthook_replace(plthook, "baa", (void *)baa_hook, (void *)&org_baa) != 0) {
        printf("plthook_replace error: %s\n", plthook_error());
        plthook_close(plthook);
        return -1;
    }
    plthook_close(plthook);
    return 0;
}

int main(int argc, char **argv) 
{ 
    install_hook_function();
    printf("%d\n", foo(10, 5));
    printf("%d\n", fof(10, 5, 8));

    print_plt_entries("./libfoo.so"); 
    return 0;
}
