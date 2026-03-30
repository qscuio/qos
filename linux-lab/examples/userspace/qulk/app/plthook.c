#include <stdio.h>
#include <stdlib.h>
#include "hmap.h"
#include "funchook.h"
#include "plthook.h"

struct hmap __hmap;

funchook_t *fhook;
funchook_info_t finfo;


void *(*orig_malloc)(int size);
void (*orig_free)(void *ptr);

void* hooked_malloc(int size)
{
	void *ptr = NULL;
	printf("Try to alloc %d\n", size);
	ptr = (*orig_malloc)(size);

	printf("Allocated %d bytes at %p\n", size, ptr);

	return ptr;
}

void hooked_free(void *ptr)
{
	printf("Try to free %p\n", ptr);

	(*orig_free)(ptr);
}

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

void hook_malloc_free(void)
{
	int ret = 0;

    plthook_t *plthook;
    ret = plthook_open(&plthook, NULL);
	if (ret != 0)
	{
		printf("open local file failed\n");
		return;
	}

    ret = plthook_replace(plthook, "malloc", (void *)hooked_malloc, (void *)&orig_malloc);
	if (ret != 0)
	{
		printf("failed to hook malloc\n");
		return;
	}
	
    ret = plthook_replace(plthook, "free", (void *)hooked_free, (void *)&orig_free);
	if (ret != 0)
	{
		printf("failed to hook free\n");
		return;
	}
    plthook_close(plthook);
}

void foo(void)
{
	char *p = malloc(10);
	free(p);
}

int main(int argc, char **argv) { 

    printf("Current working directory: %s\n", get_cwd());

    hmap_init(&__hmap);
    fhook = funchook_create();
    funchook_destroy(fhook);
    printf("++++++++++++++++++++++++++++++++++++++\n");
    print_plt_entries("userspace/lib/libw.so"); 
    printf("++++++++++++++++++++++++++++++++++++++\n");
    print_plt_entries("userspace/funchook/build/libfunchook.so.2"); 
	printf("++++++++++++++++++++++++++++++++++++++\n");
    print_plt_entries(NULL); 
	hook_malloc_free();

	foo();
	foo();
}
