#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

#define SLIST_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = SLIST_FIRST((head)); \
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1); \
	    (var) = (tvar))

struct node {
    int data;
    SLIST_ENTRY(node) next;
};

int main() {
    SLIST_HEAD(slisthead, node) head = SLIST_HEAD_INITIALIZER(head);
    struct node *node1, *node2, *node3, *node4, *temp;

    // Create nodes
    node1 = malloc(sizeof(struct node));
    node1->data = 10;
    node2 = malloc(sizeof(struct node));
    node2->data = 20;
    node3 = malloc(sizeof(struct node));
    node3->data = 30;
    node4 = malloc(sizeof(struct node));
    node4->data = 40;

    // Add nodes to the list
    SLIST_INSERT_HEAD(&head, node1, next);
    SLIST_INSERT_AFTER(node1, node2, next);
    SLIST_INSERT_AFTER(node2, node3, next);
    SLIST_INSERT_AFTER(node3, node4, next);

    // Traverse the list and print the data
    SLIST_FOREACH(temp, &head, next) {
        printf("%d ", temp->data);
    }

    // Free the memory
    SLIST_FOREACH_SAFE(temp, &head, next, node1) {
        free(temp);
    }
    printf("\n");

    return 0;
}

