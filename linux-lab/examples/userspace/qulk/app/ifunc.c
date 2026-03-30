#include <stdio.h>
#include <stdlib.h>

void foo_1()
{
    printf("This is foo1\n");
}

void foo_2()
{
    printf("This is foo2\n");
}

typedef void (*foo_t)();

void foo() __attribute__((__ifunc__("foo_resolver")));
foo_t foo_resolver()
{
    char *path;

  printf("do foo_resolver\n");
    path = getenv("FOO");
    if (path)
        return foo_1;
    else
        return foo_2;
}

void __attribute__((constructor)) initFunc(void) {
    printf("do initFunc.\n");
}

int main(int argc, char *argv[])
{
    char *env;
    printf("Do Main Func.\n");
    env = getenv("FOO");
    if (env)
        printf("do test FOO = %s\n", env);
    foo();
    return 0;
}
