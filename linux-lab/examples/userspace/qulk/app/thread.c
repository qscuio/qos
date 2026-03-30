#include <stdio.h>
#include <ovs-thread.h>

unsigned long g = 0;
static struct ovs_mutex g_lock = OVS_MUTEX_INITIALIZER;
static struct ovs_rwlock g_rw_lock = OVS_RWLOCK_INITIALIZER;

void* c_inc_thread(void *data)
{
#if 0
	unsigned int *param = (unsigned int *)data;
	bool is_single = single_threaded();
	const char *name = get_subprogram_name();
	unsigned int current = ovsthread_id_self();

	printf("Enter %s, id %d, single %d, data: %d\n", name, current, is_single, *param);
#endif
	ovs_mutex_lock(&g_lock);
	g++;
	ovs_mutex_unlock(&g_lock);

	return NULL;
}

void* c_dec_thread(void *data)
{
#if 0
	unsigned int *param = (unsigned int *)data;
	bool is_single = single_threaded();
	const char *name = get_subprogram_name();
	unsigned int current = ovsthread_id_self();

	printf("Enter %s, id %d, single %d, data: %d\n", name, current, is_single, *param);
#endif
	ovs_mutex_lock(&g_lock);
	g--;
	ovs_mutex_unlock(&g_lock);

	return NULL;
}

int c_ovs_thread(int a, int b)
{
	unsigned int i = 0;
	unsigned int count = 2000;

	for (i = 0; i < count; i++)
	{
		ovs_thread_create(__func__, c_inc_thread, &i);
		ovs_thread_create(__func__, c_dec_thread, &i);
	}

	printf("After all g == %lu\n",  g);

	return 0;
}

int main(int argc, char **argv)
{
	int cores = count_cpu_cores();
	printf("There are %d cpu in this system\n", cores);

	ovs_mutex_init(&g_lock);
	ovs_rwlock_init(&g_rw_lock);
	c_ovs_thread(10, 10);

	return 0;
}
