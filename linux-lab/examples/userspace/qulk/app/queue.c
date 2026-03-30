#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *test_queue[2];

enum msg_type {
	MSG_INT,
	MSG_STRING,
};

struct message {
	QUEUE queue;
	enum msg_type type;
	void *data;
};

int main(int argc, char **argv)
{
	int i = 0;
	int count = 0;
	int *p = 0;
	struct message *m = NULL;
	struct message *t = NULL;
	QUEUE *q = NULL;
	QUEUE *tmp = NULL;
	QUEUE_INIT(&test_queue);

	for (i = 0; i < 10; i++)
	{
		m = malloc(sizeof(struct message));
		if (m == NULL)
			return 0;

		m->data = malloc(sizeof (int));
		memcpy(m->data, &i, sizeof(int));

		QUEUE_INSERT_TAIL(&test_queue, &m->queue);
	}

	QUEUE_FOREACH(q, &test_queue)
	{
	    t = QUEUE_DATA(q, struct message, queue);
	    i = *((int *)(t->data));
	    printf("%d\n", i);
	}

#if 0
	while (!QUEUE_EMPTY(&test_queue)) {
		q = QUEUE_HEAD(&test_queue);
		QUEUE_REMOVE(q);

		t = QUEUE_DATA(q, struct message, queue);
		free(t->data);
		free(t);
		q = NULL;
		count++;

		if (count > 5)
		    break;
	}
#endif

	QUEUE_FOREACH_SAFE(q, &test_queue, tmp)
	{
		QUEUE_REMOVE(q);

		t = QUEUE_DATA(q, struct message, queue);
		free(t->data);
		free(t);
		q = NULL;
		count++;

		if (count > 5)
			break;
	}

	QUEUE_FOREACH(q, &test_queue)
	{
		t = QUEUE_DATA(q, struct message, queue);
		i = *((int *)(t->data));
		printf("%d\n", i);
	}

	return 0;
}
