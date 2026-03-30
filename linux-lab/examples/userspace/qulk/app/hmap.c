#include <stdio.h>
#include "hmap.h"

int hmap(int capacity)
{
	struct hmap *hmap = NULL;
	//struct hmap_node *new = NULL;

	hmap = xmalloc(sizeof (struct hmap));
	if (!hmap)
	{
		printf("Cannot allocate memory for hmap\n");
		return -1;
	}

	hmap_init(hmap);

	printf("Created new hash map:\n:");
	printf("Capacity: %lu\n", hmap_capacity(hmap));
	printf("Count: %lu\n", hmap_count(hmap));
	printf("IsEmpty: %d\n", hmap_is_empty(hmap));

	return 0;

}

int main(int argc, char **argv)
{
	hmap(10);
}
