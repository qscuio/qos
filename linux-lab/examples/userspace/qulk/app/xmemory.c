#include "lib.h"
#include "vlog.h"
#include "xmemory.h"

VLOG_DEFINE_THIS_MODULE(vlog_xmemory);

DEFINE_MGROUP(XMEMORY, "Program for test memory api");

DEFINE_MTYPE_STATIC(XMEMORY, TYPE1,  "memory type1");
DEFINE_MTYPE_STATIC(XMEMORY, TYPE2,  "memory type2");
DEFINE_MTYPE_STATIC(XMEMORY, TYPE3,  "memory type3");

int c_xmemory(int a, int b, int c)
{
	int i = 0;
	int *pa, *pb, *pc;

	pa = XMALLOC(MTYPE_TYPE1, a);
	pb = XMALLOC(MTYPE_TYPE2, b);
	pc = XMALLOC(MTYPE_TYPE3, c);

	XFREE(MTYPE_TYPE1, pa);
	XFREE(MTYPE_TYPE2, pb);
	XFREE(MTYPE_TYPE3, pc);

	for (i = 0; i < 20; i++)
	{
		pa = XMALLOC(MTYPE_TYPE1, a);
		pb = XMALLOC(MTYPE_TYPE2, b);
		pc = XMALLOC(MTYPE_TYPE3, c);
	}

	return a+b+c;
}

int main(int argc, char **argv)
{
	FILE *fp = fdopen(1, "w+");
	c_xmemory(10, 20, 30);

	log_memstats(fp, "xmemory");

	return 0;
}
