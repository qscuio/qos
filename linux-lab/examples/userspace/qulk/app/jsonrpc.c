#include "jsonrpc.h"
#include "lib.h"
#include "ovs-thread.h"

VLOG_DEFINE_THIS_MODULE(vlog_jsonrpc);

DEFINE_MGROUP(JSONRPC, "Program for test JSONRPC");

void *c_json_rpc_client(void *data) {
    struct jsonrpc_msg *    rq;
    struct jsonrpc_session *c =
        jsonrpc_session_open("tcp:127.0.0.1:8888", true);
	for (;;)
	{
		poll_timer_wait(1000);
		poll_block();
		jsonrpc_session_run(c);
		rq = jsonrpc_create_request(
				"A", json_array_create_1(json_boolean_create(true)), NULL);
		jsonrpc_session_send(c, rq);
	}

    return 0;
}

void *c_json_rpc_server(void *data) {
    size_t                  bl    = 0;
    struct jsonrpc_msg *    rq    = NULL;

    struct jsonrpc_session *s =
        jsonrpc_session_open("ptcp:8888:127.0.0.1", true);
	jsonrpc_session_run(s);
    VLOG_INFO("Session is alive %d\n", jsonrpc_session_is_alive(s));

    for (;;) {
		jsonrpc_session_run(s);

        jsonrpc_session_wait(s);

		rq = jsonrpc_session_recv(s);
		if (rq != NULL)
			VLOG_INFO("Recieved message from client 3 %p, bl %ld", rq, bl);
        jsonrpc_msg_destroy(rq);
    }
    return NULL;
}

int main(int argc, char **argv) {
	/* This RPC frame work does not support multiple client connect at the same time. */
    ovs_thread_create(__func__, c_json_rpc_server, NULL);
    ovs_thread_create(__func__, c_json_rpc_client, NULL);

	vlog_set_levels(NULL, VLF_ANY_DESTINATION, VLL_DBG);
	poll_timer_wait(5000);
	poll_block();
	return 0;
}
