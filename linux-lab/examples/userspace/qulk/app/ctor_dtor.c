/*
 * Copyright (C) 2006  Trevor Woerner
 */

#include <stdio.h>

void my_ctor (void) __attribute__ ((constructor));
void my_dtor (void) __attribute__ ((destructor));

void
my_ctor (void)
{
        printf ("hello before main()\n");
}

void
my_dtor (void)
{
        printf ("bye after main()\n");
}

int
main (void)
{
        printf ("hello\nbye\n");
        return 0;
}
