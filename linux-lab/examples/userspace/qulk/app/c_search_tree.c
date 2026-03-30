#define _GNU_SOURCE
#include <search.h>
#include <stdio.h>
#include <stdlib.h>

int compare(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

void print_node(const void *node, VISIT order, int depth) {
    if (order == preorder || order == leaf) {
        printf("%d\n", **(int **)node);
    }
}

int main() {
    void *root = NULL;
    int numbers[] = { 5, 2, 8, 1, 3 };
    for (int i = 0; i < 5; i++) {
        tsearch(&numbers[i], &root, compare);
    }

    twalk(root, print_node);

    int key = 3;
    if (tfind(&key, &root, compare)) {
        printf("Found: %d\n", key);
    }

    tdelete(&key, &root, compare);
    twalk(root, print_node);

    return 0;
}

