#include <unistd.h>
#include "seq.h"
#include "poll-loop.h"
#include "ovs-thread.h"

struct seq *slock = NULL;

void *update_thread(void *data)
{
    while (1)
    {
        seq_change(slock);
	usleep(10);
    }
}

void *access_thread(void *data)
{
    uint64_t seqno = seq_read(slock);
    uint64_t seqno_new = seq_read(slock);
    while(1)
    {
	seqno = seq_read(slock);
	if (seqno_new != seqno)
	{
	    printf("seqno updated observed %ld\n", seqno);
	}

	seq_wait(slock, seqno_new+1);
	seqno_new = seq_read(slock);
	if (seqno >= 100)
	    exit(0);
    }
}

int main(int argc, char **argv)
{
    slock = seq_create();

    ovs_thread_create("accessor1", access_thread, NULL);
    ovs_thread_create("accessor2", access_thread, NULL);
    ovs_thread_create("updator1", update_thread, NULL);
    ovs_thread_create("updator2", update_thread, NULL);
    ovs_thread_create("updator3", update_thread, NULL);

    poll_block();

    return 0;
}
