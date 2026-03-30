#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "injector.h"
#include "util.h"

int main(int argc, char** argv)
{
    injector_t *injector;
    void *handle;

    pid_t pid = get_pid_by_name("injector_target");

    if (injector_attach(&injector, pid)!=0){
	printf("ATTACHERROR:%s\n", injector_error());
	return 0;
    }

#if 0
    /*injectasharedlibraryintotheprocess.*/
    if (injector_inject(injector, "../lib/libw.so", NULL)!=0){
	printf("INJECTERROR:%s\n", injector_error());
    }

    /*injectanothersharedlibrary.*/
    if (injector_inject(injector,"../funchook/build/libfunchook.so.2", &handle)!=0){
	printf("INJECTERROR:%s\n", injector_error());
    }

    if (injector_inject(injector,"../plthook/libplthook_elf.so", &handle)!=0){
	printf("INJECTERROR:%s\n", injector_error());
    }
#endif

   /*injectanothersharedlibrary.*/
    if (injector_inject(injector,"../plthook/libfh.so", &handle)!=0){
	printf("INJECTERROR:%s\n", injector_error());
    }

#if 0
    /*uninjectthesecondsharedlibrary.*/
    if (injector_uninject(injector, handle)!=0){
	printf("UNINJECTERROR:%s\n", injector_error());
    }
#endif

    /*cleanup*/
    injector_detach(injector);

    return 0;
}
