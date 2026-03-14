#include "poll-loop.h"
#include "ovs-thread.h"
#include "latch.h"

void* c_latch_set_thread(void *data)
{
	int count = 0;
	struct latch *l = (struct latch *)data;

	for (;;)
	{
		count++;
		latch_set(l);
		poll_timer_wait(100);
		poll_block();
		printf("Running latch set\n");

		if (count > 5)
			break;
	}
	return NULL;
}

void* c_latch_wait_thread(void *data)
{
	struct latch *l = (struct latch *)data;
	latch_wait(l);

	return NULL;
}

int c_poll_timer(int a, int b)
{
	poll_timer_wait(1000);
	poll_timer_wait(1000 * a);
	poll_timer_wait(1000 * b);
	return 0;
}

int c_poll_event(int a, int b)
{
	poll_immediate_wake();
	return 0;
}

int c_socket_event(int a, int b)
{
	return 0;
}

int c_poll_loop(struct latch *l)
{
	int init = 0;
	int count = 0;

	while(1)
	{	
		if (!init)
		{
			/* In every poll loop, we can only register one timer.
			 * The one with the smallest value will win. 
			 * For the following six timer, only the first one takes effect.
			 */
			poll_timer_wait(1000);
			poll_timer_wait(2000);
			poll_timer_wait(3000);
			poll_timer_wait(4000);
			poll_timer_wait(5000);
			poll_timer_wait(10000);

			init = 1;
		}
		count++;
		poll_block();
		printf("Totoal 7 event, running %d\n", count);
		break;
	}

	count = 0;

	/* This poll-lopp implementation is thread safe.
	 * This poll-loop implementation is not callback based.
	 * The loop just monitor the active event, but does no process
	 * the event.
	 */
	for (;;)
	{
		poll_timer_wait(500);

		poll_fd_wait(l->fds[0], POLLIN);
		if (latch_is_set(l))
		{
			printf("Latch is set\n");
			latch_poll(l);
		}
		else
		{
			printf("Latch is not set\n");
		}
		poll_block();
		printf("Running %d poll\n", count++);

		if (count > 10)
			break;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct latch l = {0};

	latch_init(&l);

	ovs_thread_create(__func__, c_latch_set_thread, &l);
	c_poll_loop(&l);

	return 0;
}
