/*
 * Copyright (C) 2006  Trevor Woerner
 */

#include <stdio.h>

typedef void (*funcptr_t)(void);
extern funcptr_t __start_newsect, __stop_newsect;

#define data_attr         __attribute__ ((section ("newsect")))
#define create_entry(fn)  funcptr_t _##fn data_attr = fn

void my_init1 (void) { printf ("my_init1() #1\n"); }
void my_init2 (void) { printf ("my_init2() #2\n"); }

create_entry (my_init1);
create_entry (my_init2);

int
main (void)
{
    funcptr_t *call_p;

    call_p = &__start_newsect;
    do {
	printf ("call_p: %p\n", call_p);
	(*call_p)();
	++call_p;
    } while (call_p < &__stop_newsect);

    return 0;
}
