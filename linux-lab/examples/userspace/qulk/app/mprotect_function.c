#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <malloc.h>

extern int errno;

void sighdl(int s) {
	printf("Handling signal.\n");
	_exit(1);
}

#define PAGE_SIZE (0xffffffff << 12)
int function(int f) {
    //printf("[%s] is called\n", __func__);
    return f + 7;
}

int function2(int f) {
    //printf("[%s] is called\n", __func__);
    return f + 7;
}

int main() {
	int (*newf)(int) = NULL;
	int function_size = ((unsigned long int)&function2) - ((unsigned long int)&function);
	signal(SIGSEGV, sighdl);

	newf = (int (*)(int))memalign(sysconf(_SC_PAGESIZE), function_size);
	memcpy(newf, &function, function_size);

	printf("&sighdl: %p\n", &sighdl);
	printf("Page size is %ld\n", sysconf(_SC_PAGESIZE));
	printf("&function: %p\n", &function);
	printf("&function: %p\n", (void*)(((unsigned long int)&function) & (PAGE_SIZE)));
	if (-1 == mprotect(newf,
		//(void*)(((unsigned long int)&function) & (PAGE_SIZE)),
		((unsigned long int)&function2) - ((unsigned long int)&function),
		PROT_READ | PROT_EXEC)) {
		printf("mmap: %s\n", strerror(errno));
	}
	printf("function(7): %d\n", (*newf)(7));
	return 0;
}
