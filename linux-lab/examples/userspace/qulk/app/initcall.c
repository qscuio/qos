/*
 * Copyright (C) 2006  Trevor Woerner
 */

#include <stdio.h>

// ld --verbose

typedef int (*initcall_t)(void);
extern initcall_t __initcall_start, __initcall_end;

#define __initcall(fn) \
        static initcall_t __initcall_##fn __init_call = fn
#define __init_call     __attribute__ ((unused,__section__ ("function_ptrs")))
#define module_init(x)  __initcall(x);

#define __init __attribute__ ((__section__ ("code_segment")))

static int __init
my_init1 (void)
{
        printf ("my_init () #1\n");
        return 0;
}

static int __init
my_init2 (void)
{
        printf ("my_init () #2\n");
        return 0;
}

module_init (my_init1);
module_init (my_init2);

void
do_initcalls (void)
{
        initcall_t *call_p;

        call_p = &__initcall_start;
        do {
                fprintf (stderr, "call_p: %p\n", call_p);
                (*call_p)();
                ++call_p;
        } while (call_p < &__initcall_end);
}

int
main (void)
{
        fprintf (stderr, "in main()\n");
        do_initcalls ();
        return 0;
}
