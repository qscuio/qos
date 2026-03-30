#include "queue.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct msg_queue_head
{
    QUEUE queue;
    uint32_t len;
};

struct msg_queue_node
{
    QUEUE queue;
    void *data;
    int failed_action;
};

#define MSG_QUEUE_DEFINE(name)                                        \
	struct msg_queue_head name##_queue = {                        \
	    .len = 0,                                                 \
	    .queue = { &name##_queue.queue, &name##_queue.queue },    \
	}

#define MSG_QUEUE_INSERT_TAIL(name, node) do { QUEUE_INSERT_TAIL(&name##_queue.queue, &node->queue); name##_queue.len++; } while(0)
#define MSG_QUEUE_INSERT_HEAD(name, node) do { QUEUE_INSERT_HEAD(&name##_queue.queue, &node->queue); name##_queue.len++: } while(0)
#define MSG_QUEUE_REMOVE(name, node)      do { QUEUE_REMOVE(&node->queue); name##_queue.len--; } while(0)
#define MSG_QUEUE_EMPTY(name)             QUEUE_EMPTY(&name##_queue.queue)
#define MSG_QUEUE_LENGTH(name)            name##_queue.len

#define MSG_QUEUE_FOREACH(name, node)                                                         \
    for ((node) = QUEUE_DATA(QUEUE_NEXT(&name##_queue.queue),                                 \
		struct msg_queue_node, queue);                                                \
		&(node)->queue != &name##_queue.queue;                                        \
		(node) = QUEUE_DATA(QUEUE_NEXT(&(node)->queue),                               \
		    struct msg_queue_node, queue))

#define MSG_QUEUE_FOREACH_SAFE(name, node, tmp_node)                                          \
    for ((node) = QUEUE_DATA(QUEUE_NEXT(&name##_queue.queue),                                 \
                struct msg_queue_node, queue),                                                \
            (tmp_node) = QUEUE_DATA(QUEUE_NEXT(&(node)->queue),                               \
                struct msg_queue_node, queue);                                                \
                &(node)->queue != &name##_queue.queue;                                        \
                (node) = (tmp_node),                                                          \
            (tmp_node) = QUEUE_DATA(QUEUE_NEXT(&(node)->queue),                               \
                struct msg_queue_node, queue))

#define MSG_QUEUE_FLUSH(name)                                                                 \
    do {                                                                                      \
	struct msg_queue_node *___node__ = NULL;                                              \
	struct msg_queue_node *___temp__ = NULL;                                              \
	MSG_QUEUE_FOREACH_SAFE(name, ___node__, ___temp__)                                    \
	{                                                                                     \
	    MSG_QUEUE_REMOVE(name, ___node__);                                                \
	    if (___node__->data)                                                              \
	    free(___node__->data);                                                            \
	    free(___node__);                                                                  \
	}                                                                                     \
    } while(0)

#define MSG_QUEUE_INSERT_DATA_HEAD(name, _data)                                               \
    do  {                                                                                     \
	struct msg_queue_node *__new__ = malloc(sizeof (struct msg_queue_node));             \
	if (__new__)                                                                         \
	{                                                                                     \
	    memset(__new__, 0, sizeof(struct msg_queue_node));                               \
	    __new__->data = _data;                                                           \
	    MSG_QUEUE_INSERT_HEAD(name, __new__);                                            \
	}                                                                                     \
    } while(0)

#define MSG_QUEUE_INSERT_DATA_TAIL(name, _data)                                               \
    do  {                                                                                     \
	struct msg_queue_node *__new__ = malloc(sizeof (struct msg_queue_node));             \
	if (__new__)                                                                         \
	{                                                                                     \
	    memset(__new__, 0, sizeof(struct msg_queue_node));                               \
	    __new__->data = _data;                                                           \
	    MSG_QUEUE_INSERT_TAIL(name, __new__);                                            \
	}                                                                                     \
    } while(0)

#define MSG_QUEUE_FOREACH_MSG(name, callback, priv)                                           \
    do {                                                                                      \
	struct msg_queue_node *__node__ = NULL;                                               \
	struct msg_queue_node *__temp__ = NULL;                                               \
	MSG_QUEUE_FOREACH_SAFE(name, __node__, __temp__)                                      \
	{                                                                                     \
	    (*callback)(__node__->data, priv);                                                \
	}                                                                                     \
    } while (0)

#define MSG_QUEUE_REMOVE_MSG(name, msg, compare)                                              \
    do {                                                                                      \
	struct msg_queue_node *__node__ = NULL;                                              \
	struct msg_queue_node *__temp__ = NULL;                                              \
	MSG_QUEUE_FOREACH_SAFE(name, __node__, __temp__)                                    \
	{                                                                                     \
	    if (!(*compare)(__node__->data, msg))                                             \
	    {                                                                                 \
                MSG_QUEUE_REMOVE(name, __node__);                                             \
		free(__node__->data);                                                         \
		free(__node__);                                                               \
	    }                                                                                 \
	}                                                                                     \
    } while (0)


#define MSG_QUEUE_STR(str) #str
#define MSG_QUEUE_APP_NAME MSG_QUEUE_STR(__progname)

MSG_QUEUE_DEFINE(test);
MSG_QUEUE_DEFINE(MSG_QUEUE_APP_NAME);

int compare_int(void *msg, void *key)
{
    int *a = (int *)msg;
    int *b = (int *)key;

    return *a == *b ? 0 : 1;
}

void dump_int(void *data, void *priv)
{
    int *j = (int *)data;

    printf("%d\n", *j);
}

int main(int argc, char **argv)
{
    int i = 0;
    int count = 0;
    int *j = 0;
    struct msg_queue_node *new = NULL;
    struct msg_queue_node *tmp = NULL;
    struct msg_queue_node *node = NULL;

    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));

    for (i = 0; i < 20; i++)
    {
	new = malloc(sizeof(struct msg_queue_node));
	new->data = malloc(sizeof(int));
	memcpy(new->data, &i, sizeof(int));
	MSG_QUEUE_INSERT_TAIL(test, new);
    }

    MSG_QUEUE_FOREACH(test, node)
    {
	printf("%d\n", *((int *)(node->data)));
    }
    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));

    MSG_QUEUE_FOREACH_SAFE(test, node, tmp)
    {
	MSG_QUEUE_REMOVE(test, node);
	free(node->data);
	free(node);
	count++;
	if (count > 5)
	    break;
    }

    printf("-----After remove------\n");
    MSG_QUEUE_FOREACH(test, node)
    {
	printf("%d\n", *((int *)(node->data)));
    }
    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));

    MSG_QUEUE_FOREACH_SAFE(test, node, tmp)
    {
	MSG_QUEUE_REMOVE(test, node);
	free(node->data);
	free(node);
	count++;
	if (count > 10)
	    break;
    }

    printf("-----After delete------\n");
    MSG_QUEUE_FOREACH(test, node)
    {
	printf("%d\n", *((int *)(node->data)));
    }
    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));

    MSG_QUEUE_FLUSH(test);

    printf("-----After flush------\n");
    MSG_QUEUE_FOREACH(test, node)
    {
	printf("%d\n", *((int *)(node->data)));
    }
    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));

    for (i = 0; i < 20; i++) 
    {
	j = malloc(sizeof(int));
	memcpy(j, &i, sizeof(int));
	MSG_QUEUE_INSERT_DATA_TAIL(test, j);
    }

    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));

    int b = 18;
    MSG_QUEUE_REMOVE_MSG(test, &b, compare_int);
    b = 10;
    MSG_QUEUE_REMOVE_MSG(test, &b, compare_int);
    MSG_QUEUE_FOREACH_MSG(test, dump_int, NULL);
    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));

    MSG_QUEUE_FOREACH_MSG(test, dump_int, NULL);

    MSG_QUEUE_FLUSH(test);
    printf("-----After flush------\n");
    MSG_QUEUE_FOREACH(test, node)
    {
	printf("%d\n", *((int *)(node->data)));
    }
    printf("head %p, len, %d, cap: %d, queue[0] %p, queue[1] %p, empty %d\n", test_queue.queue, test_queue.len, 0, test_queue.queue[0], test_queue.queue[1], MSG_QUEUE_EMPTY(test));


    return 0;
}
