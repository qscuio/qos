/* CPM (Common Protocol/Process Messaging) client. */
#include "lib.h"
#include "xthread.h"
#include "message.h"
#include "cpm_client.h"
#include "vlog.h"

VLOG_DEFINE_THIS_MODULE(vlog_cpm_client);

DEFINE_MGROUP(XMEMORY_CPM_CLIENT, "CPM Client");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_CLIENT, CPM_PENDING_MSG,  "Pending Message");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_CLIENT, CPM_CLIENT,  "CPM Client");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_CLIENT, CPM_CLIENT_HANDLER,  "CPM Client Handler");

void
cpm_client_id_set (struct cpm_client *cc, struct cpm_msg_service *service)
{
  if ((cc->async != NULL) &&
      (cc->async->service.client_id != service->client_id))
    cc->async->service.client_id = service->client_id;
}


void
cpm_client_id_unset (struct cpm_client *cc)
{
  if (cc->async)
    cc->async->service.client_id = 0;
}


void
cpm_client_set_version (struct cpm_client *cc, u_int16_t version)
{
  cc->service.version = version;
}


void
cpm_client_set_protocol (struct cpm_client *cc, u_int32_t protocol_id)
{
  cc->service.protocol_id = protocol_id;
}

void
cpm_client_set_parser (struct cpm_client *cc, int message_type,
                       CPM_PARSER parser)
{

  cc->parser[message_type] = parser;
}

/* Register callback.  */
void
cpm_client_set_callback (struct cpm_client *cc, int message_type,
                         CPM_CALLBACK callback)
{
  cc->callback[message_type] = callback;
}


/* Register disconnect callback. */
void
cpm_client_set_disconnect_callback (struct cpm_client *cc,
                                    CPM_DISCONNECT_CALLBACK callback)
{
  cc->disconnect_callback = callback;
}


static int32_t
cpm_client_reconnect (struct thread *t)
{
  struct cpm_client *cc;

  cc = THREAD_ARG (t);
  cc->t_connect = NULL;
  cpm_client_start (cc);

  return 0;
}


/*
 * Start to connect CPM services.
 */
int32_t
cpm_client_start (struct cpm_client *cc)
{
    int ret;

    if (cc->async)
    {
	ret = message_client_start (cc->async->mc);
	if (ret < 0)
	{
	    /* Start reconnect timer.  */
	    if (cc->t_connect == NULL)
	    {
		thread_add_timer (cc->zg->master, cpm_client_reconnect,
			cc, cc->reconnect_interval, &cc->t_connect);
		if (cc->t_connect == NULL)
		    return -1;
	    }
	}
    }
    return 0;
}


/* Stop CPM client. */
void
cpm_client_stop (struct cpm_client *cc)
{
  if (cc->async)
    message_client_stop (cc->async->mc);
}


/* Reconnect to the server. */
static int32_t
cpm_client_reconnect_start (struct cpm_client *cc)
{
    /* Start reconnect timer.  */
    thread_add_timer (cc->zg->master, cpm_client_reconnect,
	    cc, cc->reconnect_interval, &cc->t_connect);
    if (cc->t_connect == NULL)
	return -1;

    return 0;
}


/* Client connection is established.
 * Client sends service description message to the server.
 */
static int32_t
cpm_client_connect (struct message_handler *mc, struct message_entry *me,
                    sock_handle_t sock)
{
  struct cpm_client_handler *cch = mc->info;
  struct thread t;

  cch->up = 1;

#if 0
  /* Send service message to NSM server.  */
  nsm_client_send_service (cch);
#endif/*0*/

  if (cch->cc->ccb == NULL)
    return -1;

  /*
   * Let owner send service message to its server.
   */
  (*cch->cc->ccb) (cch);

  /* Always read service message synchronously */
  THREAD_ARG (&t) = mc;
  THREAD_FD (&t) = mc->sock;
  message_client_read (&t);

  return 0;
}


static int32_t
cpm_client_disconnect (struct message_handler *mc, struct message_entry *me,
                       sock_handle_t sock)
{
  struct cpm_client_handler *cch;
  struct cpm_client *cc;
  struct listnode *node, *next;

  cch = mc->info;
  cc = cch->cc;

  /* Set status to down.  */
  cch->up = 0;

  /* Clean up client ID.  */
  cpm_client_id_unset (cc);

  /* Cancel pending read thread. */
  if (cch->pend_read_thread)
    THREAD_OFF (cch->pend_read_thread);

  /* Free all pending reads. */
  for (node = LISTHEAD (&cch->pend_msg_list); node; node = next)
    {
      struct cpm_client_pend_msg *pmsg = GETDATA (node);

      next = node->next;

      XFREE (MTYPE_CPM_PENDING_MSG, pmsg);
      list_delete_node (&cch->pend_msg_list, node);
    }

  /* Stop async connection.  */
  if (cc->async)
    message_client_stop (cc->async->mc);

  /* Unset the shutdown flag */
  UNSET_FLAG (cc->cpm_server_flags, CPM_SERVER_SHUTDOWN);

  /* Call client specific disconnect handler. */
  if (cc->disconnect_callback)
    cc->disconnect_callback ();
  else
    cpm_client_reconnect_start (cc);

  return 0;
}


static int32_t
cpm_client_read (struct cpm_client_handler *cch, sock_handle_t sock)
{
  struct cpm_msg_header *header;
  u_int16_t length;
  int nbytes = 0;

  cch->size_in = 0;
  cch->pnt_in = cch->buf_in;

  nbytes = readn (sock, cch->buf_in, CPM_MSG_HEADER_SIZE);
  if (nbytes < CPM_MSG_HEADER_SIZE)
    return -1;

  header = (struct cpm_msg_header *) cch->buf_in;
  length = ntohs(header->length);

  nbytes = readn (sock, cch->buf_in + CPM_MSG_HEADER_SIZE,
                  length - CPM_MSG_HEADER_SIZE);
  if (nbytes <= 0)
    return nbytes;

  cch->size_in = length;

  return length;
}


/*
 * Asynchronous CPM message read function.
 * This function must be called at the beginning of the owner's
 * read message function
 */
int32_t
cpm_client_read_msg (struct message_handler *mc,
                     struct message_entry *me,
                     sock_handle_t sock)
{
  struct cpm_client_handler *cch;
  struct cpm_client *cc;
  struct cpm_msg_header header;
  int nbytes;
  int type;
  int ret;

  /* Get CPM client handler from message entry. */
  cch = mc->info;
  cc = cch->cc;

  /* Read data. */
  nbytes = cpm_client_read (cch, sock);
  if (nbytes <= 0)
    return nbytes;

  /* Parse CPM message header. */
  ret = cpm_decode_header (&cch->pnt_in, &cch->size_in, &header);
  if (ret < 0)
    return -1;

  /* Dump CPM header. */
  if (cc->debug)
    cpm_header_dump (mc->zg, &header);

  memcpy(&cch->header, &header, sizeof(struct cpm_msg_header));
  type = header.type;

  /* Invoke parser with call back function pointer.  There is no callback
     set by protocols for MPLS replies. */
  if (type < cc->msg_max && cc->parser[type] && cc->callback[type])
    {
      ret = (*cc->parser[type]) (&cch->pnt_in, &cch->size_in, &header, cch,
                                 cc->callback[type]);
      if (ret < 0)
        VLOG_DBG ("Parse error for message %d", type);
    }

  return nbytes;
}


/*
 * Synchronous CPM message read function.
 */
int32_t
cpm_client_read_sync (struct message_handler *mc, struct message_entry *me,
                      sock_handle_t sock,
                      struct cpm_msg_header *header, int *type)
{
  struct cpm_client_handler *cch;
  struct cpm_client *cc;
  int nbytes;
  int ret;

  /* Get CPM server entry from message entry.  */
  cch = mc->info;
  cc = cch->cc;

  /* Read msg */
  nbytes = cpm_client_read (cch, sock);
  if (nbytes <= 0)
    {
      message_client_disconnect (mc, sock);
      return -1;
    }

  /* Parse CPM message header.  */
  ret = cpm_decode_header (&cch->pnt_in, &cch->size_in, header);
  if (ret < 0)
    return -1;

  /* Dump CPM header.  */
  if (cc->debug)
    cpm_header_dump (mc->zg, header);

  *type = header->type;

  return nbytes;
}


void
cpm_client_pending_message (struct cpm_client_handler *cch,
                            struct cpm_msg_header *header)
{
  struct cpm_client_pend_msg *pmsg;

  /* Queue the message for later processing. */
  pmsg = XMALLOC (MTYPE_CPM_PENDING_MSG, sizeof (struct cpm_client_pend_msg));
  if (pmsg == NULL)
    return;

  pmsg->header = *header;

  memcpy (pmsg->buf, cch->pnt_in, cch->size_in);

  /* Add to pending list. */
  listnode_add (&cch->pend_msg_list, pmsg);
}


/*
 * Process pending message requests.
 */
int32_t
cpm_client_process_pending_msg (struct thread *t)
{
  struct cpm_client_handler *cch;
  struct cpm_client *cc;
  struct abc *zg;
  struct listnode *node;
  u_int16_t size;
  u_char *pnt;
  int ret;

  cch = THREAD_ARG (t);
  cc = cch->cc;

  /* Reset thread. */
  zg = cch->cc->zg;
  cch->pend_read_thread = NULL;

  node = LISTHEAD (&cch->pend_msg_list);
  if (node)
    {
      struct cpm_client_pend_msg *pmsg = GETDATA (node);
      int type;

      if (pmsg == NULL)
        return 0;

      pnt = pmsg->buf;
      size = pmsg->header.length - CPM_MSG_HEADER_SIZE;
      type = pmsg->header.type;

      if (cc->debug)
        cpm_header_dump (zg, &pmsg->header);

      if (cc->debug)
        VLOG_INFO ("Processing pending message %d", type);

      list_delete_node (&cch->pend_msg_list, node);
      cch->pend_read_proc = 1;
      ret = (*cc->parser[type]) (&pnt, &size, &pmsg->header, cch,
                                 cc->callback[type]);
      cch->pend_read_proc = 0;
      if (ret < 0)
        VLOG_DBG ("Parse error for message %d", type);

      /* Free processed message and node. */
      XFREE (MTYPE_CPM_PENDING_MSG, pmsg);
    }
  else
    return 0;

  node = LISTHEAD (&cch->pend_msg_list);
  if (node)
    {
      /* Process pending requests. */
      if (!cch->pend_read_thread)
          thread_add_read (zg->master, cpm_client_process_pending_msg, cch, 0, &cch->pend_read_thread);
    }

  return 0;
}

/*
 * Generic function to send message to CPM server.
 */
int32_t
cpm_client_send_message (struct cpm_client_handler *cch,
                         int type, u_int16_t len, u_int32_t *msg_id)
{
  struct cpm_msg_header header;
  u_int16_t size;
  u_char *pnt;
  int ret;

  if (! cch || ! cch->up)
    return -1;

  if (cch->cc == NULL)
    return -1;
  
  /* check if cpm server is in shutdown state */
  if (CHECK_FLAG (cch->cc->cpm_server_flags, CPM_SERVER_SHUTDOWN))
    return 0;

  /* HA specific handling is done by application code */

  pnt = cch->buf;
  size = CPM_MESSAGE_MAX_LEN;

  /* If message ID warparounds, start from 1. */
  ++cch->message_id;
  if (cch->message_id == 0)
    cch->message_id = 1;

  /* Prepare CPM message header.  */
  header.type = type;
  header.length = len + CPM_MSG_HEADER_SIZE;
  header.message_id = cch->message_id;

  /* Encode header.  */
  cpm_encode_header (&pnt, &size, &header);

  /* Write message to the socket.  */
  ret = writen (cch->mc->sock, cch->buf, len + CPM_MSG_HEADER_SIZE);
  if (ret != len + CPM_MSG_HEADER_SIZE)
    return -1;

  if (msg_id)
    *msg_id = header.message_id;

  return 0;
}


static struct cpm_client_handler *
cpm_client_handler_create (struct cpm_client *cc, int type)
{
  struct cpm_client_handler *cch;
  struct message_handler *mc;

  /* Allocate CPM client handler.  */
  cch = XCALLOC (MTYPE_CPM_CLIENT_HANDLER, cc->h_size);
  if (cch == NULL)
    return NULL;

  cch->type = type;
  cch->cc = cc;

  /* Set max message length.  */
  cch->len = CPM_MESSAGE_MAX_LEN;
  cch->len_in = CPM_MESSAGE_MAX_LEN;

  cch->pend_read_thread = NULL;

  /* Create async message client. */
  mc = message_client_create (cc->zg, type);

#ifndef HAVE_TCP_MESSAGE
  /* Use UNIX domain socket connection.  */
  message_client_set_style_domain (mc, cc->path);
#else /* HAVE_TCP_MESSAGE */
  message_client_set_style_tcp (mc, cc->port);
#endif /* !HAVE_TCP_MESSAGE */

  /* Initiate connection using CPM connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               cpm_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               cpm_client_disconnect);
  message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
                               cpm_client_read_msg);

  /* Link each other.  */
  cch->mc = mc;
  mc->info = cch;

  cch->pnt = cch->buf;
  cch->pnt_in = cch->buf_in;

  if (cc->hccb != NULL)
    (*cc->hccb)(cch);

  return cch;
}

static int32_t
cpm_client_handler_free (struct cpm_client_handler *cch)
{
  if (cch->cc->hfcb != NULL)
    (*cch->cc->hfcb)(cch);

  if (cch->mc)
    message_client_delete (cch->mc);

  XFREE (MTYPE_CPM_CLIENT_HANDLER, cch);

  return 0;
}

/* Set service type flag.  */
int32_t
cpm_client_set_service (struct cpm_client *cc, int service)
{
  int type;

  if (service >= cc->cpm_service_type_max)
    return -1;

  /* Set service bit to CPM client.  */
  CPM_SET_CTYPE (cc->service.bits, service);

  /* Check the service is sync or async.  */
  type = cc->cpm_service_type[service].type;

  /* Create client hander corresponding to message type.  */
  if (type == MESSAGE_TYPE_ASYNC)
    {
      if (cc->async == NULL)
        {
          cc->async = cpm_client_handler_create (cc, MESSAGE_TYPE_ASYNC);
          cc->async->service.version = cc->service.version;
          cc->async->service.protocol_id = cc->service.protocol_id;
        }
      CPM_SET_CTYPE (cc->async->service.bits, service);
    }

  return 0;
}


/* Initialize CPM client.
 * This function allocate CPM client memory.
 *
 * Input:
 *   owner_id: owner ID (e.g., CPM_OWNER_RIBD)
 *   owner_client_size:
 *             size of owner's client data structure
 *             (e.g., sizeof(struct rib_client))
 *   owner_client_handler_size:
 *             size of owner's client_handler data structure
 *             (e.g., sizeof(struct rib_client_handler))
 *   msg_max:  max # of owner's parser/callback functions
 *   handler_create_cb:
 *             function pointer wherein (*hccb)() is called when
 *             cpm_client_handler_create() is called.
 *   handler_create_cb:
 *             function pointer wherein (*hfcb)() is called when
 *             owner's client_handler is freed.
 *   connect_cb:
 *             function pointer wherein (*ccb)() is called when
 *             owner's client connected to its server.
 *             This function must send its own service message
 *             to the server. The server side service callback function
 *             must call cpm_server_recv_service() at the beginning.
 *             Refer to nsm_client_connect(), nsm_client_send_service(),
 *             nsm_server_recv_service() for more details how to implement
 *             callback functions.
 */
struct cpm_client *
cpm_client_create (struct abc *zg,
                   u_int16_t owner_id,
                   char *path,
                   u_int16_t port,
                   u_int16_t owner_client_size,
                   u_int16_t owner_client_handler_size,
                   u_int16_t msg_max,
                   cpm_client_handler_cb handler_create_cb,
                   cpm_client_handler_cb handler_free_cb,
                   cpm_client_handler_cb connect_cb,
                   u_int32_t debug)
{
  struct cpm_client *cc;

#ifndef HAVE_TCP_MESSAGE
  if (path == NULL)
    return NULL;
#endif /* !HAVE_TCP_MESSAGE */

  cc = XCALLOC (MTYPE_CPM_CLIENT, owner_client_size);
  if (cc == NULL)
    return NULL;

#ifdef HAVE_TCP_MESSAGE
  cc->port = port;
#else
  cc->path = XCALLOC (MTYPE_CPM_CLIENT, strlen (path) + 1);
  strcpy(cc->path, path);
#endif /* !HAVE_TCP_MESSAGE */

  cc->zg = zg;

  cc->owner_id = owner_id;
  cc->c_size = owner_client_size;
  cc->h_size = owner_client_handler_size;
  cc->msg_max = msg_max;
  cc->hccb = handler_create_cb;
  cc->hfcb = handler_free_cb;
  cc->ccb = connect_cb;
  cc->parser = NULL;
  cc->callback = NULL;
  cc->reconnect_interval = 5;

  if (debug)
    cc->debug = 1;

  return cc;
}


int32_t
cpm_client_delete (struct cpm_client *cc)
{
  struct listnode *node, *node_next;
  struct cpm_client_handler *cch;

  if (cc != NULL)
    {
      if (cc->t_connect)
        THREAD_OFF (cc->t_connect);

      cch = cc->async;
      /* Free all pending reads. */
      if (cch)
        {
          /* Cancel pending read thread. */
          if (cch->pend_read_thread)
            THREAD_OFF (cch->pend_read_thread);

          for (node = LISTHEAD (&cch->pend_msg_list); node; node = node_next)
            {
              struct cpm_client_pend_msg *pmsg = GETDATA (node);

              node_next = node->next;

              XFREE (MTYPE_CPM_PENDING_MSG, pmsg);
              list_delete_node (&cch->pend_msg_list, node);
            }

          cpm_client_handler_free (cch);
        }

      XFREE (MTYPE_CPM_CLIENT, cc);
    }
  return 0;
}
