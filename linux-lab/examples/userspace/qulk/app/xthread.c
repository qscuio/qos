#include "xthread.h"
#include "xmemory.h"
#include "vlog.h"
#include "latch.h"

VLOG_DEFINE_THIS_MODULE(vlog_xthread);

DEFINE_MGROUP(XMEMORY, "Program for for test xthread");

DEFINE_MTYPE_STATIC(XMEMORY, XTYPE1,  "memory xtype1");
DEFINE_MTYPE_STATIC(XMEMORY, XTYPE2,  "memory xtype2");
DEFINE_MTYPE_STATIC(XMEMORY, XTYPE3,  "memory xtype3");

struct thread_master *master = NULL;


#if 0
/* dummy task for sleeper pipe */
static int thread_dummy(struct thread *thread)
{
    return 0;
}
#endif

int thread_latch_read(struct thread *thread)
{
	static int count = 0;
	struct latch *l = THREAD_ARG(thread);
	thread = NULL;

	VLOG_INFO("Recevied message on latch %d limit %d\n", l->fds[0], master->fd_limit);
	latch_poll(l);
	if (count < 3)
	{
		thread_add_read(master, &thread_latch_read, l, l->fds[0], NULL);
	}
    return 0;
}

int thread_latch_write(struct thread *thread)
{
	struct latch *l = THREAD_ARG(thread);
	thread = NULL;

	VLOG_INFO("Send message on latch %d, limit %d\n", l->fds[1], master->fd_limit);
	latch_set(l);
    return 0;
}

int thread_timer(struct thread *thread)
{
	static int count = 0;
	VLOG_INFO("Timer expired\n");
	struct latch *l = THREAD_ARG(thread);
	thread = NULL;

	if (count < 3)
	{
		thread_add_write(master, &thread_latch_write, l, l->fds[1], NULL);
		thread_add_timer(master, &thread_timer, l, 5, NULL);
	}
	count++;
    return 0;
}

int thread_timer_msec(struct thread *thread)
{
	thread = NULL;
	VLOG_INFO("Msec Timer expired\n");
	exit(0);
    return 0;
}

int c_xthread(int a, int b)
{
	struct latch l = {0};
	master = thread_master_create(__func__);

	struct thread thread;
#if 0
	int sleeper[2];

	pipe(sleeper);
	thread_add_read(master, &thread_dummy, NULL, sleeper[0], NULL);
#endif

	latch_init(&l);

	thread_add_read(master, &thread_latch_read, &l, l.fds[0], NULL);
	thread_add_write(master, &thread_latch_write, &l, l.fds[1], NULL);
	thread_add_timer(master, &thread_timer, &l, 5, NULL);
	thread_add_timer_msec(master, &thread_timer_msec, NULL, 20000, NULL);

	while(thread_fetch(master, &thread))
		thread_call(&thread);

	return a + b;
}

int main(int argc, char **argv)
{
	c_xthread(10, 10);
	return 0;
}
