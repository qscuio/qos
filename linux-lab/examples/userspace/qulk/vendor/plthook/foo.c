#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bar.h"
#include "foo.h"

int foo(int a, int b)
{
    return bar(a, b);
}

int fof(int a, int b, int c)
{
    return baa(a, b, c);
}

void call_printf(void)
{
    printf("[%s:%d]\n", __func__, __LINE__);
}

void call_write(void)
{
    write(1, "hello world\n", strlen("hello world\n"));
}

