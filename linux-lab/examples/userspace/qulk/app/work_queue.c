#include "lib.h"
#include "vlog.h"
#include "queue.h"
#include "xthread.h"
#include "workqueue.h"
#include "frr_pthread.h"

VLOG_DEFINE_THIS_MODULE(vlog_workqueue);

DEFINE_MGROUP(XWORKQUEUE, "Program for frr workqueue api");

DEFINE_MTYPE_STATIC(XWORKQUEUE, CITEM,  "Consumer Item Memory");
DEFINE_MTYPE_STATIC(XWORKQUEUE, WORK_ITEM,  "Work Item Memory");


struct work_queue *Q = NULL;
struct thread_master *tm = NULL; 
static int counter = 0;

struct citem {
	STAILQ_ENTRY(citem) item;
	void *data;
};
STAILQ_HEAD(queue_items, citem) remain_items;
static int citem_qlen = 0;

static int thread_dummy(struct thread *thread)
{
	return 0;
}

static void work_queue_done(struct work_queue *wq)
{
	VLOG_INFO("Working queue finished\n");
	return;
}

static void work_queue_error(struct work_queue *wq, struct work_queue_item *item)
{
	int *i = (int *)item->data;

	VLOG_INFO("%d process has failed\n", *i);
	return;
}

static void work_queue_del_item(struct work_queue *wq, void *data)
{
	int *i = (int *)data;

	VLOG_INFO("Delete item %d\n", *i);
	XFREE(MTYPE_WORK_ITEM, data);

	return;
}

static wq_item_status work_item_process(struct work_queue *wq, void *data)
{
	int *m = NULL;
	int *i = (int *)data;

	struct citem *c = NULL;

	if (citem_qlen > 4)
		return WQ_RETRY_LATER;

	if (citem_qlen > 8)
		return WQ_QUEUE_BLOCKED;

	c = XMALLOC(MTYPE_CITEM, sizeof (struct citem));

	m = XMALLOC(MTYPE_WORK_ITEM, 4);
	*m = *i;

	c->data = m;

	STAILQ_INSERT_TAIL(&remain_items, c, item);
	citem_qlen++;

	VLOG_INFO("Process item %d\n", *i);

#if 0
	if (*i % 3)
		return WQ_REQUEUE;
	else if (*i % 4)
		return WQ_RETRY_LATER;
	else if (*i % 5)
		return WQ_QUEUE_BLOCKED;
#endif

	return WQ_SUCCESS;
	//return WQ_REQUEUE;
	//return WQ_RETRY_LATER;
	//return WQ_QUEUE_BLOCKED;
}

static int thread_workqueue_timer(struct thread *thread)
{
	int i = 0;
	int *m = NULL;

	for (i = 0; i < 1; i++)
	{
		m = XMALLOC(MTYPE_WORK_ITEM, 4);
		*m = counter++;
		work_queue_add(Q, m);
	}
	VLOG_INFO("Currrent Queue Length %d\n", work_queue_item_count(Q));
	thread_add_timer(tm, &thread_workqueue_timer, NULL, 1, NULL);
    return 0;
}

static int thread_workqueue_consumer_timer(struct thread *thread)
{
	int *i = 0;

	struct citem *c = NULL;

	if (STAILQ_EMPTY(&remain_items))
	{
		VLOG_INFO("consumer queue is empty\n");
		thread_add_timer(tm, &thread_workqueue_consumer_timer, NULL, 2, NULL);
		return 0;
	}

	c =(struct citem *)STAILQ_FIRST(&remain_items);
	i = (int *)c->data;
	VLOG_INFO("%d is consumed current qlen %d\n", *i, citem_qlen);

	STAILQ_REMOVE(&remain_items, c, citem, item);
	XFREE(MTYPE_WORK_ITEM, c->data);
	XFREE(MTYPE_CITEM, c);
	citem_qlen--;
	thread_add_timer(tm, &thread_workqueue_consumer_timer, NULL, 4, NULL);
    return 0;
}

static int thread_workqueue_start(struct thread *thread)
{
	Q = work_queue_new(tm, __func__);
	Q->spec.workfunc = work_item_process;
	Q->spec.errorfunc = work_queue_error;
	Q->spec.del_item_data = work_queue_del_item;
	Q->spec.completion_func= work_queue_done;
	Q->spec.max_retries = 3;
	thread_add_timer(tm, &thread_workqueue_timer, NULL, 1, NULL);
	return 0;
}

int main(int argc, char **argv)
{
	struct thread task;
	int sleeper[2];

	STAILQ_INIT(&remain_items);

	tm = thread_master_create(__func__);

	pipe(sleeper);

	thread_add_event(tm, thread_workqueue_start, NULL, 0, NULL);
	thread_add_read(tm, &thread_dummy, NULL, sleeper[0], NULL);
	thread_add_timer(tm, &thread_workqueue_consumer_timer, NULL, 2, NULL);
	while (thread_fetch(tm , &task))
		thread_call(&task);
	return 0;
}
