#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>  // Required for hcreate_r, hsearch_r, hdestroy_r

int main() {
	int ret = 0;
    struct hsearch_data htab;

	/* This is necessary. */
	memset(&htab, 0, sizeof(htab));
    
    ret = hcreate_r(10, &htab);
	if (ret == 0)
	{
        fprintf(stderr, "Error creating hash table %d\n", ret);
        return EXIT_FAILURE;
    }

    // Define an entry to be added to the hash table
    ENTRY item;
    ENTRY *found_item;
    item.key = "key1";
    item.data = "value1";
    
    // Insert the item into the hash table
    if (hsearch_r(item, ENTER, &found_item, &htab) == 0) {
        fprintf(stderr, "Error adding item to hash table\n");
        hdestroy_r(&htab);
        return EXIT_FAILURE;
    }

    ENTRY item2;
    item2.key = "key2";
    item2.data = "value2";
    
    // Insert the item into the hash table
    if (hsearch_r(item2, ENTER, &found_item, &htab) == 0) {
        fprintf(stderr, "Error adding item to hash table\n");
        hdestroy_r(&htab);
        return EXIT_FAILURE;
    }


    // Search for an item in the hash table
    item.key = "key1";
    if (hsearch_r(item, FIND, &found_item, &htab) != 0) {
        printf("Found: %s -> %s\n", found_item->key, (char*)found_item->data);
    } else {
        printf("Item not found\n");
    }

    // Search for an item in the hash table
    item.key = "key2";
    if (hsearch_r(item, FIND, &found_item, &htab) != 0) {
        printf("Found: %s -> %s\n", found_item->key, (char*)found_item->data);
    } else {
        printf("Item not found\n");
    }

    // Destroy the hash table
    hdestroy_r(&htab);
    
    return EXIT_SUCCESS;
}

