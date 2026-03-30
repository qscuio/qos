#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#define TAILQ_FOREACH_SAFE(var, head, field, tvar)         \
    for ((var) = TAILQ_FIRST((head));                      \
	    (var) && ((tvar) = TAILQ_NEXT((var), field), 1);   \
	    (var) = (tvar))

// Define a structure for a node in the queue
struct queue_node {
    int data;
    TAILQ_ENTRY(queue_node) entries;
};

TAILQ_HEAD(queue_head, queue_node); // Declare a queue head

int main() {
    struct queue_head head; // Declare a queue head
    TAILQ_INIT(&head); // Initialize the queue

    // Create some nodes and add them to the queue
    struct queue_node *node1 = malloc(sizeof(struct queue_node));
    node1->data = 10;
    TAILQ_INSERT_TAIL(&head, node1, entries);

    struct queue_node *node2 = malloc(sizeof(struct queue_node));
    node2->data = 2;
    TAILQ_INSERT_TAIL(&head, node2, entries);

    struct queue_node *node3 = malloc(sizeof(struct queue_node));
    node3->data = 1000;
    TAILQ_INSERT_TAIL(&head, node3, entries);


    struct queue_node *node4 = malloc(sizeof(struct queue_node));
    node4->data = 4;
    TAILQ_INSERT_HEAD(&head, node4, entries);

    struct queue_node *node5 = malloc(sizeof(struct queue_node));
    node5->data = 1000;
    TAILQ_INSERT_TAIL(&head, node5, entries);


    // Traverse the queue in forward direction
    struct queue_node *current_node;
    TAILQ_FOREACH(current_node, &head, entries) {
        printf("%d ", current_node->data);
    }
    printf("\n");

    // Remove the second node from the queue
    TAILQ_REMOVE(&head, node2, entries);
    free(node2);

    // Traverse the queue in reverse direction
    TAILQ_FOREACH_REVERSE(current_node, &head, queue_head, entries) {
        printf("%d ", current_node->data);
    }
    printf("\n");

/*
    // Free the remaining nodes in the queue
    while (!TAILQ_EMPTY(&head)) {
        current_node = TAILQ_FIRST(&head);
        TAILQ_REMOVE(&head, current_node, entries);
        free(current_node);
    }
*/

    struct queue_node *tmp;
    TAILQ_FOREACH_SAFE(current_node, &head, entries, tmp) {
	printf("Removing node with data %d\n", current_node->data);
	TAILQ_REMOVE(&head, current_node, entries);
	free(current_node);
    }

    TAILQ_FOREACH(current_node, &head, entries) {
        printf("%d ", current_node->data);
    }
    printf("\n");

    return 0;
}

