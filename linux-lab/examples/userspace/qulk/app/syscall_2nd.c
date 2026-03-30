#include <syscall.h>
#include <stdio.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <stdlib.h>

int main(void)
{
	printf("%ld\n",syscall(335));
	return 0;
}
