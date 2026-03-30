#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

struct node {
    int data;
    SIMPLEQ_ENTRY(node) entries;
};

#define SIMPLEQ_FOREACH_SAFE(var, head, field, tvar)                                 \
    for ((var) = SIMPLEQ_FIRST((head));                                           \
	    (var) != NULL && ((tvar) = SIMPLEQ_NEXT((var), field), 1);                \
	    (var) = (tvar))


int main() {
    SIMPLEQ_HEAD(head, node) head = SIMPLEQ_HEAD_INITIALIZER(head);
    struct node *np;

    // Add nodes to the queue
    np = malloc(sizeof(*np));
    np->data = 1;
    SIMPLEQ_INSERT_TAIL(&head, np, entries);

    np = malloc(sizeof(*np));
    np->data = 2;
    SIMPLEQ_INSERT_TAIL(&head, np, entries);

    np = malloc(sizeof(*np));
    np->data = 3;
    SIMPLEQ_INSERT_TAIL(&head, np, entries);

    // Traverse the queue
    printf("Queue contents: ");
    SIMPLEQ_FOREACH(np, &head, entries) {
        printf("%d ", np->data);
    }
    printf("\n");

#if 0
    // Remove nodes from the queue
    while (!SIMPLEQ_EMPTY(&head)) {
        np = SIMPLEQ_FIRST(&head);
        SIMPLEQ_REMOVE_HEAD(&head, entries);
        free(np);
    }
#endif
    struct node *tmp;
    SIMPLEQ_FOREACH_SAFE(np, &head, entries, tmp) {
        printf("Removing node with data %d\n", np->data);
        SIMPLEQ_REMOVE_HEAD(&head, entries);
        free(np);
    }

    // Traverse the queue
    printf("Queue contents: ");
    SIMPLEQ_FOREACH(np, &head, entries) {
        printf("%d ", np->data);
    }
    printf("\n");

    return 0;
}

