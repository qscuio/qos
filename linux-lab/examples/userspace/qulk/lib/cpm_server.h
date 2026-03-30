#ifndef __HAVE_CPM_SERVER_H__
#define __HAVE_CPM_SERVER_H__

#include "cpm_client.h"

/*
 * CPM (Common Protocol/Process Messaging) Server definitions.
 */
struct cpm_server;
struct cpm_server_entry;

/*
 * CPM server side callback function typedefs.
 */
typedef void (*cpm_server_cb) (struct cpm_server *cs);
typedef void (*cpm_server_entry_cb) (struct cpm_server_entry *cse);


/*
 * CPM Send/Recv data structure for server entry.
 */
struct cpm_server_entry_buf
{
  u_char buf[CPM_MESSAGE_MAX_LEN];
  u_int16_t len;

  u_char *pnt;
  u_int16_t size;
};

/*
 * CPM Server buffer structure for handling bulk messages.
 */
struct cpm_server_bulk_buf
{
    /* Buffer used to send bulk message */
    struct cpm_server_entry_buf bulk_buf;

    /* Timer for handling server bulk buffer.  */
    struct thread *bulk_thread;
};
#define CPM_SERVER_BUNDLE_TIME         1


/*
 * CPM (Common Protocol Messaging) server structure
 */
/* CPM server structure.  */
struct cpm_server
{
  struct abc *zg;

  struct message_handler *ms;

  /* CPM service structure.  */
  struct cpm_msg_service service;

  /* Buffer for handling Bulk Messages from NSM server. */
  struct cpm_server_bulk_buf server_buf;

  /* Vector for all of CPM server client.  */
  vector client;

  /* # of parser and callback functions */
  u_int16_t msg_max;

  /* Owner's server size in bytes (e.g., struct rib_server) */
  u_int16_t s_size;

  /* Owner's entry size in bytes (e.g., struct rib_server_entry) */
  u_int16_t e_size;

  /* Owner's client size in bytes (e.g., struct rib_server_client) */
  u_int16_t sc_size;

  /* pointer to owner's parser function pointer array */
  CPM_PARSER *parser;

  /* pointer to owner's callback function pointer array */
  CPM_CALLBACK *callback;

  /* Callback functions */
  cpm_server_entry_cb ccb;      /* connect */
  cpm_server_entry_cb dccb;     /* disconnect */
  cpm_server_entry_cb sefcb;    /* server_entry free */

  /* Debug flag.  */
  int debug;

#if 0
@@@
@@@ Owner specific members
@@@
  /* Parser functions.  */
  NSM_PARSER parser[NSM_MSG_MAX];

  /* Call back functions.  */
  NSM_CALLBACK callback[NSM_MSG_MAX];

  /* COMMMSG recv callback. */
  nsm_msg_commsg_recv_cb_t  ns_commsg_recv_cb;
  void                     *ns_commsg_user_ref;
#endif/*0*/
};

/*
 * CPM server client.
 */
struct cpm_server_client
{
  /* CPM client ID. */
  u_int32_t client_id;

  /* NSM server entry for sync connection.  */
  struct cpm_server_entry *head;
  struct cpm_server_entry *tail;

#if 0
@@@
@@@ Owner specific information
@@@
  /* NSM storing BGP state information */
  u_int32_t conv_state;

  /* NHT enabled/disabled flag
     if NHT is enabled this variable would be PAL_TRUE
     else the value of variable would be PAL_FALSE */
  u_int8_t nhtenable;
#endif/*0*/
};

/*
 * CPM server entry for each client connection.
 */
struct cpm_server_entry
{
  /* Linked list. */
  struct cpm_server_entry *next;
  struct cpm_server_entry *prev;

  /* Pointer to message entry.  */
  struct message_entry *me;

  /* Pointer to CPM server structure.  */
  struct cpm_server *cs;

  /* Pointer to CPM server client.  */
  struct cpm_server_client *csc;

  /* CPM service structure.  */
  struct cpm_msg_service service;

  /* Send/Recv buffers. */
  struct cpm_server_entry_buf send;
  struct cpm_server_entry_buf recv;

  /* Send and recieved message count.  */
  u_int32_t send_msg_count;
  u_int32_t recv_msg_count;

  /* Connect time.  */
  time_t connect_time;

  /* Last read time.  */
  time_t read_time;

  /* Message queue.  */
  struct fifo send_queue;
  struct thread *t_write;

  /* Message id */
  u_int32_t message_id;

  /* For record.  */
  u_int16_t last_read_type;
  u_int16_t last_write_type;

#if 0
@@@
@@@ Owner specific information
@@@
  /* Message buffer for IPv4 redistribute.  */
  u_char buf_ipv4[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv4;
  u_int16_t len_ipv4;
  struct thread *t_ipv4;

#ifdef HAVE_IPV6
  /* Message buffer for IPv6 redistribute.  */
  u_char buf_ipv6[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv6;
  u_int16_t len_ipv6;
  struct thread *t_ipv6;
#endif /* HAVE_IPV6 */

  /* Redistribute request comes from this client.  */
  struct nsm_redistribute *redist;
#endif/*0*/
};


/*
 * CPM Server public functions
 */
struct cpm_server_client *
cpm_server_recv_service (struct cpm_msg_header *header,
                         void *arg, void *message);
struct cpm_server *
cpm_server_init (struct abc *zg,
                 char *path,
                 u_int16_t port,
                 u_int16_t s_size,
                 u_int16_t e_size,
                 u_int16_t sc_size,
                 cpm_server_entry_cb ccb,
                 cpm_server_entry_cb dccb,
                 cpm_server_entry_cb sefcb
                 );
int32_t
cpm_server_finish (struct cpm_server *cs, void *cs_appln);
int32_t
cpm_server_send_message (struct cpm_server_entry *cse,
                         int32_t type, u_int32_t msg_id, u_int16_t len);

void
cpm_server_enqueue (struct cpm_server_entry *cse, u_char *buf,
                    u_int16_t length, u_int16_t written);

/* Set protocol version.  */
void cpm_server_set_version (struct cpm_server *cs, u_int16_t version);

/* Set protocol ID.  */
void cpm_server_set_protocol (struct cpm_server *cs, u_int32_t protocol_id);

void 
cpm_service_dump (struct abc *zg, struct cpm_msg_service *service);

#endif /* __HAVE_CPM_SERVER_H__ */
