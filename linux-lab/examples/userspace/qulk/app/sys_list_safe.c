#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

struct node {
    int data;
    LIST_ENTRY(node) entries;
};

#define LIST_FOREACH_SAFE(var, head, field, tvar) \
  for ((var) = LIST_FIRST((head)); \
       (var) && ((tvar) = LIST_NEXT((var), field), 1); \
       (var) = (tvar))


int main() {
    LIST_HEAD(listhead, node) head = LIST_HEAD_INITIALIZER(head);
    struct node *np;

    // Add nodes to the list
    np = malloc(sizeof(*np));
    np->data = 1;
    LIST_INSERT_HEAD(&head, np, entries);

    np = malloc(sizeof(*np));
    np->data = 2;
    LIST_INSERT_HEAD(&head, np, entries);

    np = malloc(sizeof(*np));
    np->data = 3;
    LIST_INSERT_HEAD(&head, np, entries);

    // Traverse the list
    printf("List contents: ");
    LIST_FOREACH(np, &head, entries) {
        printf("%d ", np->data);
    }
    printf("\n");


    // Traverse the list and remove nodes safely
    struct node *tmp;
    LIST_FOREACH_SAFE(np, &head, entries, tmp) {
        printf("Removing node with data %d\n", np->data);
        LIST_REMOVE(np, entries);
        free(np);
    }

    // Traverse the list
    printf("List contents: ");
    LIST_FOREACH(np, &head, entries) {
        printf("%d ", np->data);
    }
    printf("\n");



    return 0;
}

