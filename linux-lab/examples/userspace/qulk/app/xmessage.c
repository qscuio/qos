#include "lib.h"
#include "vlog.h"
#include "ovs-thread.h"
#include "poll-loop.h"
#include "xthread.h"
#include "message.h"

VLOG_DEFINE_THIS_MODULE(vlog_xmessage);

#define TEST_MESSAGE 1

struct abc X;
struct thread_master *master = NULL;

int c_server_callback(struct message_handler *h, struct message_entry *e, sock_handle_t fd)
{
	VLOG_INFO("Server received message \n");
	return 0;
}

int c_client_callback(struct message_handler *h, struct message_entry *e, sock_handle_t fd)
{
	VLOG_INFO("client received message \n");
	return 0;
}

int c_server_connect_callback(struct message_handler *h, struct message_entry *e, sock_handle_t fd)
{
	VLOG_INFO("Server received connect message \n");
	return 0;
}

int c_client_connect_callback(struct message_handler *h, struct message_entry *e, sock_handle_t fd)
{
	VLOG_INFO("client received connect message \n");
	return 0;
}

void* c_message_server(void *data)
{
	struct message_handler *h = message_server_create(&X);
	message_server_set_style_tcp(h, 9999);
	VLOG_INFO("[%s][%d]--\n", __func__, __LINE__);
	message_server_set_callback(h, MESSAGE_EVENT_CONNECT, c_server_connect_callback);
	message_server_set_callback(h, TEST_MESSAGE, c_server_callback);
	message_server_start(h);
	VLOG_INFO("[%s][%d]--\n", __func__, __LINE__);

	return NULL;
}

void* c_message_client(void *data)
{
	struct message_handler *h = message_client_create(&X, MESSAGE_TYPE_ASYNC);
	message_client_set_style_tcp(h, 9999);
	VLOG_INFO("[%s][%d]--\n", __func__, __LINE__);
	message_client_set_callback(h, MESSAGE_EVENT_CONNECT, c_client_connect_callback);
	message_client_set_callback(h, TEST_MESSAGE, c_client_callback);
	message_client_start(h);
	message_client_read_register(h);
	VLOG_INFO("[%s][%d]--\n", __func__, __LINE__);

	return NULL;
}

int thread_timer_msec(struct thread *thread)
{
	thread = NULL;
	VLOG_INFO("Msec Timer expired\n");
	exit(0);
    return 0;
}

int main(int argc, char **argv)
{
	struct thread thread;
	/* First start server */
	ovs_thread_create(__func__, c_message_server, NULL);
	poll_timer_wait(1000);
	poll_block();

	/* Then start client */
	ovs_thread_create(__func__, c_message_client, NULL);
	VLOG_INFO("[%s][%d]--\n", __func__, __LINE__);
	VLOG_INFO("[%s][%d]--\n", __func__, __LINE__);

	thread_add_timer_msec(master, &thread_timer_msec, NULL, 20000, NULL);

	while(thread_fetch(master, &thread))
		thread_call(&thread);
}
