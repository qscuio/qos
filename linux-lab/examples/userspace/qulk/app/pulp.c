#define _GNU_SOURCE
#include <dlfcn.h>
#include <gnu/lib-names.h>
#include <gnu/libc-version.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ld_rtld.h"
#include "ulp.h"

int main(int argc, char **argv)
{
    load_so("../funchook/build/libfunchook.so.2");
    load_patch();

    return 0;
}
