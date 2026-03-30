#include "ovs-thread.h"
#include "vlog.h"
#include "poll-loop.h"

VLOG_DEFINE_THIS_MODULE(atomic_vlog);

#define THREAD_COUNT  2000

void* c_atomic_inc(void *data)
{
	int i = 0;
	atomic_count *count = (atomic_count *)data;

	for (i = 0; i < THREAD_COUNT; i++)
		atomic_count_inc(count);

	return NULL;
}

void* c_atomic_dec(void *data)
{
	int i = 0;
	atomic_count *count = (atomic_count *)data;

	for (i = 0; i < THREAD_COUNT; i++)
		atomic_count_dec(count);

	return NULL;
}

void* c_int_inc(void *data)
{
	int i = 0;
	int *count = (int *)data;

	for (i = 0; i < THREAD_COUNT; i++)
		*count += 1;

	return NULL;
}

void* c_int_dec(void *data)
{
	int i = 0;
	int *count = (int *)data;

	for (i = 0; i < THREAD_COUNT; i++)
	*count -= 1;

	return NULL;
}

int main(int argc, char **argv)
{
	int i = 0;
	int icount = 0;
	uint32_t curr = 0;
	atomic_count count;

	curr = atomic_count_get(&count);
	VLOG_INFO("%s: Not init %u\n", __func__, curr);
	atomic_count_init(&count, 0);

	curr = atomic_count_get(&count);
	VLOG_INFO("%s: After init  %u\n", __func__, curr);

	for (i = 0; i < THREAD_COUNT; i++)
	{
		ovs_thread_create(__func__, c_atomic_inc, &count);
		ovs_thread_create(__func__, c_atomic_dec, &count);
	}

	/* wait previous thread finish */
	poll_timer_wait(5000);
	poll_block();

	curr = atomic_count_get(&count);
	VLOG_INFO("%s: After running atomic %u\n", __func__, curr);

	for (i = 0; i < THREAD_COUNT; i++)
	{
		ovs_thread_create(__func__, c_int_inc, &icount);
		ovs_thread_create(__func__, c_int_dec, &icount);
	}

	/* wait previous thread finish */
	poll_timer_wait(5000);
	poll_block();
	VLOG_INFO("%s: After running int %d\n", __func__, icount);
}
