#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <search.h>

int main() {
    // Create a hash table with space for 10 elements
    if (hcreate(10) == 0) {
        fprintf(stderr, "Error creating hash table\n");
        return EXIT_FAILURE;
    }

    // Define and insert an entry into the hash table
    ENTRY item;
    item.key = "key1";
    item.data = "value1";
    ENTRY *found = hsearch(item, ENTER);

    if (found != NULL) {
        printf("Inserted: %s -> %s\n", found->key, (char*)found->data);
    } else {
        printf("Failed to insert item\n");
    }

    // Search for an entry in the hash table
    item.key = "key1";
    found = hsearch(item, FIND);
    if (found != NULL) {
        printf("Found: %s -> %s\n", found->key, (char*)found->data);
    } else {
        printf("Item not found\n");
    }

    // Destroy the hash table
    hdestroy();
    
    return EXIT_SUCCESS;
}

