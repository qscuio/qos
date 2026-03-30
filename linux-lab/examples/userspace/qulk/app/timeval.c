#include <stdio.h>
#include "timeval.h"

int c_time_val(int a, int b)
{
	int usage = get_cpu_usage();

	printf("cpu usage %d\n", usage);

	return 0;
}

int main(int argc, char **argv)
{
	c_time_val(10, 10);
	return 0;
}
