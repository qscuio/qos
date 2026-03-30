#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

void add(long a, long b)
{
    printf("a+b=%ld\n", a + b);
}

char *get_call_addr(char *p, int len)
{
    while (len-- && (*p != (char) 0xe8))
	p++;

    if (len)
	return p;
    else
	return 0;
}

void set_call(long call_addr, long fun_addr)
{
    long ip = call_addr + 5;
    long set_call_addr = call_addr + 1;
    (*(long *) set_call_addr) = fun_addr - ip;
}


int main()
{
    char *p = (char *) malloc(100);
    mprotect((void *) ((size_t) p - (size_t) p % 4096), 4096,
	     PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(p, add, 100);

    void (*pf) (long, long) = (void (*)(long, long)) p;

    char *call_addr = get_call_addr(p, 100);
    if (call_addr) {
	set_call((long) call_addr, (long) printf);
	pf(10, 20);
    }

    return 0;
}
