/*
 * CPM (Common Protocol/Process Messaging)
 * server implementation.
 */
#include "lib.h"
#include "xthread.h"
#include "tlv.h"
#include "vector.h"
#include "message.h"
#include "cpm_server.h"
#include "vlog.h"
#include "xmemory.h"

VLOG_DEFINE_THIS_MODULE(vlog_cpm_server);

DEFINE_MGROUP(XMEMORY_CPM_SERVER, "CPM Server");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_SERVER, CPM_SERVER,  "CPM Server");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_SERVER, CPM_SERVER_ENTRY,  "CPM Server Entry");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_SERVER, CPM_SERVER_CLIENT,  "CPM Server Client");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_SERVER, CPM_MSG_QUEUE_BUF,  "CPM Message Queue Buffer");
DEFINE_MTYPE_STATIC(XMEMORY_CPM_SERVER, CPM_MSG_QUEUE,  "CPM Message Qeueue");

void
cpm_header_dump (struct abc *zg, struct cpm_msg_header *header)
{
  VLOG_INFO ("CPM Message Header");
  VLOG_INFO (" Message type: %d", header->type);
  VLOG_INFO (" Message length: %d", header->length);
  VLOG_INFO (" Message ID: 0x%08x", header->message_id);
}

void
cpm_service_dump (struct abc *zg, struct cpm_msg_service *service)
{
  VLOG_INFO ("CPM Service");
  VLOG_INFO (" Version: %d", service->version);
  VLOG_INFO (" Owner: %d", service->owner_id);
  VLOG_INFO (" Protocol ID: (%d)", service->protocol_id);
  VLOG_INFO (" Client ID: %d", service->client_id);
  VLOG_INFO (" Bits: %d", service->bits);
}


/* Set protocol version.  */
void
cpm_server_set_version (struct cpm_server *cs, u_int16_t version)
{
  cs->service.version = version;
}


/* Set protocol ID.  */
void
cpm_server_set_protocol (struct cpm_server *cs, u_int32_t protocol_id)
{
  cs->service.protocol_id = protocol_id;
}


/*
 * Free CPM server entry.
 */
void
cpm_server_entry_free (struct cpm_server_entry *cse)
{
  struct cpm_message_queue *queue;

  while ((queue
          = (struct cpm_message_queue *) FIFO_TOP (&cse->send_queue)) != NULL)
    {
      FIFO_DEL (queue);
      XFREE (MTYPE_CPM_MSG_QUEUE_BUF, queue->buf);
      XFREE (MTYPE_CPM_MSG_QUEUE, queue);
    }

  /* Turn off the write thread */
  THREAD_OFF (cse->t_write);

  /*
   * Let an owner do its own clean-up
   */
  if (cse->cs->sefcb != NULL)
    (*cse->cs->sefcb)(cse);

  XFREE (MTYPE_CPM_SERVER_ENTRY, cse);
}


/* Client connect to CPM server.
 */
static int32_t
cpm_server_connect (struct message_handler *ms, struct message_entry *me,
                    sock_handle_t sock)
{
  struct cpm_server *cs = ms->info;
  struct cpm_server_entry *cse;

  assert (cs != NULL);

  /* Application data size should not be less than cpm_server_entry size */
  if ( sizeof (struct cpm_server_entry) > cs->e_size)
    return -1; 

  cse = XCALLOC (MTYPE_CPM_SERVER_ENTRY, cs->e_size);
  cse->send.len = CPM_MESSAGE_MAX_LEN;
  cse->recv.len = CPM_MESSAGE_MAX_LEN;

  me->info = cse;
  cse->me = me;
  cse->cs = cs;
  cse->connect_time = time(NULL);
  FIFO_INIT (&cse->send_queue);

  /*
   *  Let an owner do its own stuff
   */
  if (cs->ccb != NULL)
    (*cs->ccb) (cse);


  return 0;
}

/*
 * Client disconnect from CPM server.
 * This function should not be called directly.
 * Message library call this.
 * original : nsm_server_disconnect
 */
static int32_t
cpm_server_disconnect (struct message_handler *ms, struct message_entry *me,
                       sock_handle_t sock)
{
  struct cpm_server *cs;
  struct cpm_server_entry *cse;
  struct cpm_server_client *csc;

  cse = me->info;
  cs = cse->cs;

  /* Figure out CPM client which cse belongs to.  */
  csc = cse->csc;
  if (csc == NULL)
    {
      if (cs->debug)
        VLOG_INFO ("CPM Client Server disconnect");

      return 0;
    }

  /* Logging.  */
  if (cs->debug)
    VLOG_INFO ("CPM client ID %d disconnect", cse->service.client_id);

  /*
   *  Let an owner do its own clean-up
   */
  if (cs->dccb != NULL)
    (*cs->dccb) (cse);

  /* Remove nse from linked list.  */
  if (cse->prev)
    cse->prev->next = cse->next;
  else
    csc->head = cse->next;
  if (cse->next)
    cse->next->prev = cse->prev;
  else
    csc->tail = cse->prev;

  /* If CPM server client becomes empty free it.  */
  if ((csc->head == NULL) && (csc->tail == NULL))
    {
      vector_unset (cs->client, csc->client_id);
      XFREE (MTYPE_CPM_SERVER_CLIENT, csc);
    }

  /* Free CPM server entry.  */
  cpm_server_entry_free (cse);
  me->info = NULL;

  return 0;
}


/*
 * CPM message header encoder.
 */
int32_t
cpm_encode_header (u_char **pnt, u_int16_t *size,
                   struct cpm_msg_header *header)
{
  u_char *sp = *pnt;

  if (*size < CPM_MSG_HEADER_SIZE)
    return EC_LIB_MESSAGE_SIZE_TOO_SMALL;

  TLV_ENCODE_PUTW (header->type);
  TLV_ENCODE_PUTW (header->length);
  TLV_ENCODE_PUTL (header->message_id);

  return *pnt - sp;
}

/*
 * CPM message header decoder.
 */
int32_t
cpm_decode_header (u_char **pnt, u_int16_t *size,
                   struct cpm_msg_header *header)
{
  if (*size < CPM_MSG_HEADER_SIZE)
    return EC_LIB_MESSAGE_SIZE_TOO_SMALL;

  TLV_DECODE_GETW (header->type);
  TLV_DECODE_GETW (header->length);
  TLV_DECODE_GETL (header->message_id);

  return CPM_MSG_HEADER_SIZE;
}

/*
 * Common receive service request.
 * Owner's server must call this function when its own
 * receive service is requested.
 */
struct cpm_server_client *
cpm_server_recv_service (struct cpm_msg_header *header,
                         void *arg, void *message)
{
  struct cpm_server_client *csc = NULL;
  struct cpm_server_entry *cse = arg;
  struct cpm_server *cs = cse->cs;
  struct cpm_msg_service *service = message;
  u_int32_t index;

  /* Dump received message. */
  cse->service = *service;

  if (cs->debug)
    cpm_service_dump (cs->zg, &cse->service);

  /* When received client id is zero, we assgin new client id for it.  */
  if (cse->service.client_id)
    {
      index = service->client_id;
      if (index < vector_size (cs->client))
        csc = vector_slot (cs->client, index);
    }

  if (csc == NULL)
    {
      /* Get new client id.  This is little bit tricky in vector index
         starts from zero, but client_id should be non zero.  So we
         add one to vector index for client id.  */
      csc = XCALLOC (MTYPE_CPM_SERVER_CLIENT, cs->sc_size);
      if (csc == NULL)
        return NULL;

      /* XXX: Client_id is local to a system and therefore cannot be
       * checkpointed. But it is used for Graceful Restart mechanism to
       * mark the routes STALE based on client id.
       * Therefore, for HA the client id will be the protocol id. This will
       * be ensured by assigning the client_id as the protocol_id at time
       * of CPM client connect (in cpm_server_recv_service() ).
       */
      index = vector_set_index (cs->client, cse->service.protocol_id, csc);
      csc->client_id = index;
      cse->service.client_id = csc->client_id;
    }

  /* Add CPM client entry to CPM client list.  */
  cse->prev = csc->tail;
  if (csc->head == NULL)
    csc->head = cse;
  else
    csc->tail->next = cse;
  csc->tail = cse;

  /* Remember which CPM sever client nse belongs to.  */
  cse->csc = csc;

  /* Send back server side service bits to client.  CPM server just
     send all of service bits which server can provide to the client.
     It is up to client side to determine the service is enough or not.  */
  cs->service.cindex = 0;
  cs->service.client_id = cse->service.client_id;

  return csc;
}


/*
 * De-queue CPM message.
 */
static int32_t
cpm_server_dequeue (struct thread *t)
{
  struct cpm_server_entry *cse;
  struct cpm_message_queue *queue;
  sock_handle_t sock;
  int32_t nbytes;

  cse = THREAD_ARG (t);
  sock = THREAD_FD (t);
  cse->t_write = NULL;

  queue = (struct cpm_message_queue *) FIFO_HEAD (&cse->send_queue);
  if (queue)
    {
      nbytes = write (sock, queue->buf + queue->written,
                               queue->length - queue->written);
      if (nbytes <= 0)
        {
          if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
              VLOG_ERR ("CPM message send error socket %d %s",
                        cse->me->sock, strerror(errno));
              return -1;
            }
        }
      else if (nbytes != (queue->length - queue->written))
        {
          queue->written += nbytes;
        }
      else
        {
          FIFO_DEL (queue);
          XFREE (MTYPE_CPM_MSG_QUEUE_BUF, queue->buf);
          XFREE (MTYPE_CPM_MSG_QUEUE, queue);
        }
    }

  if (FIFO_TOP (&cse->send_queue))
    thread_add_write(cse->cs->zg->master, cpm_server_dequeue, cse,
                     cse->me->sock, &cse->t_write);

  return 0;
}


/*
 * Enqueue the message unless it couldn't write the message to the socket.
 */
void
cpm_server_enqueue (struct cpm_server_entry *cse, u_char *buf,
                    u_int16_t length, u_int16_t written)
{
  struct cpm_message_queue *queue;

  queue = XCALLOC (MTYPE_CPM_MSG_QUEUE, sizeof (struct cpm_message_queue));
  if(queue == NULL)
    {
      VLOG_ERR ("Memory for CPM message queue is exhausted");
      thread_add_write(cse->cs->zg->master, cpm_server_dequeue, cse,
                       cse->me->sock, &cse->t_write);
      return;
    }

  queue->buf = XMALLOC (MTYPE_CPM_MSG_QUEUE_BUF, length);
  if(queue->buf == NULL)
    {
      VLOG_ERR ("Memory for CPM message queue buffer is exhausted");
      thread_add_write (cse->cs->zg->master,
                       cpm_server_dequeue, cse, cse->me->sock, &cse->t_write);
      return;
    }
  memcpy (queue->buf, buf, length);
  queue->length = length;
  queue->written = written;

  FIFO_ADD (&cse->send_queue, queue);

  thread_add_write(cse->cs->zg->master, 
                   cpm_server_dequeue, cse, cse->me->sock, &cse->t_write);
}

/*
 * Send message to the client.
 */
int32_t
cpm_server_send_message (struct cpm_server_entry *cse,
                         int32_t type, u_int32_t msg_id, 
                         u_int16_t len)
{
  struct cpm_msg_header header;
  u_int16_t total_len;
  int32_t nbytes;

#if 0
@@@
@@@ TODO : Handled by called application code. Not requried here.
@@@        Delete code under if 0 for HAVE_HA
@@@
#ifdef HAVE_HA
  if ((HA_IS_STANDBY (nzg))
      && (! HA_ALLOW_NSM_MSG_TYPE(type)))
    return 0;
#endif /* HAVE_HA */
#endif/*0*/

  cse->send.pnt = cse->send.buf;
  cse->send.size = cse->send.len;

  /* Prepare CPM message header.  */
  header.type = type;
  header.length = len + CPM_MSG_HEADER_SIZE;
  header.message_id = msg_id;
  total_len = len + CPM_MSG_HEADER_SIZE;

  cpm_encode_header (&cse->send.pnt, &cse->send.size, &header);

  if (FIFO_TOP (&cse->send_queue))
    {
      cpm_server_enqueue (cse, cse->send.buf, total_len, 0);
      return 0;
    }

  /* Send message.  */
  nbytes = write (cse->me->sock, cse->send.buf, total_len);
  if (nbytes <= 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        cpm_server_enqueue (cse, cse->send.buf, total_len, 0);
      else
        {
          VLOG_ERR ("CPM message send error socket %d %s",
                    cse->me->sock, strerror(errno));
          return -1;
        }
    }
  else if (nbytes != total_len)
    cpm_server_enqueue (cse, cse->send.buf, total_len, nbytes);
  else
    {
      cse->last_write_type = type;
      cse->send_msg_count++;
    }
  return 0;
}


/*
 * Call back function to read CPM message body.
 */
static int32_t
cpm_server_read_msg (struct message_handler *ms, struct message_entry *me,
                     sock_handle_t sock)
{
  int32_t ret;
  int32_t type;
  int32_t nbytes;
  struct cpm_server *cs;
  struct cpm_server_entry *cse;
  struct cpm_msg_header header;

  memset (&header, 0, sizeof (struct cpm_msg_header));

  /* Get CPM server entry from message entry.  */
  cse = me->info;
  if (cse == NULL)
    {
      return -1;
    }

  cs = cse->cs;
  if (cs == NULL)
    {
      return -1;
    }

  /* Reset parser pointer and size. */
  cse->recv.pnt = cse->recv.buf;
  cse->recv.size = 0;

  /* Read CPM message header.  */
  nbytes = readn (sock, cse->recv.buf, CPM_MSG_HEADER_SIZE);

  /* Let message handler to handle disconnect event.  */
  if (nbytes <= 0)
    return -1;

  /*
   * Check header length. If length is smaller than CPM message
   * header size close the connection.
   */
  if (nbytes != CPM_MSG_HEADER_SIZE)
    return -1;

  /* Record read size.  */
  cse->recv.size = nbytes;

  /* Parse CPM message header.  */
  ret = cpm_decode_header (&cse->recv.pnt, &cse->recv.size, &header);
  if (ret < 0)
    return -1;

  /* Dump CPM header.  */
  if (cs->debug)
    cpm_header_dump (me->zg, &header);

  /* Reset parser pointer and size. */
  cse->recv.pnt = cse->recv.buf;
  cse->recv.size = 0;

  /* Read CPM message body.  */
  nbytes = readn (sock, cse->recv.pnt, header.length - CPM_MSG_HEADER_SIZE);

  /* Let message handler to handle disconnect event.  */
  if (nbytes <= 0 || nbytes > CPM_MESSAGE_MAX_LEN)
  {
      VLOG_ERR ("%s:%d Error message: type %d length %d\n", __func__, __LINE__, header.type, nbytes);
      return -1;
  }

  /* Record read size.  */
  cse->recv.size = nbytes;
  type = header.type;

  /* Increment counter.  */
  cse->recv_msg_count++;

  /* Put last read type.  */
  cse->last_read_type = type;

  /* Invoke call back function.  */
  if (type < cs->msg_max && cs->parser[type] && cs->callback[type])
    {
      ret = (*cs->parser[type]) (&cse->recv.pnt, &cse->recv.size,
                                 &header, cse, cs->callback[type]);
      if (ret < 0)
        return ret;
    }

  return nbytes;
}


/*
 * Initialize a CPM server.
 * Input:
 *   zg: pointer to library global
 *   s_size:  size (in bytes) of the owner's server structure
 *            (e.g., sizeof(struct rib_server))
 *   e_size:  size (in bytes) of the owner's server_entry structure
 *            (e.g., sizeof(struct rib_server_entry))
 *   sc_size:  size (in bytes) of the owner's server_client_entry structure
 *             (e.g., sizeof(struct rib_server_client_entry))
 *   ccb:      function pointer wherein (*ccb)() is called when
 *             an owner's client connected to the server.
 *   dccb:     function pointer wherein (*dccb)() is called when
 *             an owner's client disconnected to the server.
 *   sefcb:    function pointer wherein (*sefcb)() is called when
 *             an owner's server_entry structure
 *             (e.g., struct rib_server_entry) is freed.
 *   path:     
 */
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
                 )
{
  int32_t ret;
  struct cpm_server *cs;
  struct message_handler *ms;

  /* Create message server.  */
  ms = message_server_create (zg);
  if (! ms)
    return NULL;

#ifndef HAVE_TCP_MESSAGE
  if (path == NULL)
  {
    message_server_delete(ms);
    return NULL;
  }

  /* Set server type to UNIX domain socket.  */
  message_server_set_style_domain (ms, path);
#else /* HAVE_TCP_MESSAGE */
  if (port <= 1024)
  {
    message_server_delete(ms);
    return NULL;
  }

  message_server_set_style_tcp (ms, port);
#endif /* !HAVE_TCP_MESSAGE */

  /* Set call back functions.  */
  message_server_set_callback (ms, MESSAGE_EVENT_CONNECT,
                               cpm_server_connect);
  message_server_set_callback (ms, MESSAGE_EVENT_DISCONNECT,
                               cpm_server_disconnect);
  message_server_set_callback (ms, MESSAGE_EVENT_READ_MESSAGE,
                               cpm_server_read_msg);

  /* Start CPM server.  */
  ret = message_server_start (ms);
  if (ret < 0)
  {
    message_server_delete(ms);
    return NULL;
  }

  /* When message server works fine, go forward to create CPM server
     structure.  */
  cs = XCALLOC (MTYPE_CPM_SERVER, s_size);
  if (cs == NULL)
  {
    message_server_delete(ms);
    return NULL;
  }

  cs->s_size = s_size;
  cs->e_size = e_size;
  cs->sc_size = sc_size;
  cs->zg = zg;
  cs->ms = ms;
  cs->ccb = ccb;
  cs->dccb = dccb;
  cs->sefcb = sefcb;
  ms->info = cs;
  cs->client = vector_init (1);

  memset (&cs->server_buf, 0, sizeof (struct cpm_server_bulk_buf));
  cs->server_buf.bulk_buf.len= CPM_MESSAGE_MAX_LEN;

  return cs;

#if 0
@@@
@@@ Owner specific
@@@
  /* Set services.  */
  nsm_server_set_service (ns, NSM_SERVICE_INTERFACE);
  nsm_server_set_service (ns, NSM_SERVICE_ROUTE);
  nsm_server_set_service (ns, NSM_SERVICE_ROUTER_ID);
  nsm_server_set_service (ns, NSM_SERVICE_VRF);
  nsm_server_set_service (ns, NSM_SERVICE_ROUTE_LOOKUP);
  nsm_server_set_service (ns, NSM_SERVICE_LABEL);
#ifdef HAVE_MPLS_TP
  nsm_server_set_service (ns, NSM_SERVICE_MPLS_TP_GLOBALS);
#endif /* HAVE_MPLS_TP */
#ifdef HAVE_TE
  nsm_server_set_service (ns, NSM_SERVICE_TE);
#endif /* HAVE_TE */
  nsm_server_set_service (ns, NSM_SERVICE_QOS);
  nsm_server_set_service (ns, NSM_SERVICE_QOS_PREEMPT);
  nsm_server_set_service (ns, NSM_SERVICE_USER);
  nsm_server_set_service (ns, NSM_SERVICE_MPLS_VC);
  nsm_server_set_service (ns, NSM_SERVICE_MPLS);
  nsm_server_set_service (ns, NSM_SERVICE_IGP_SHORTCUT);
#ifdef HAVE_GMPLS
  nsm_server_set_service (ns, NSM_SERVICE_GMPLS);
  nsm_server_set_service (ns, NSM_SERVICE_CONTROL_ADJ);
  nsm_server_set_service (ns, NSM_SERVICE_CONTROL_CHANNEL);
  nsm_server_set_service (ns, NSM_SERVICE_TE_LINK);
  nsm_server_set_service (ns, NSM_SERVICE_DATA_LINK);
  nsm_server_set_service (ns, NSM_SERVICE_DATA_LINK_SUB);
#endif /* HAVE_GMPLS */
#ifdef HAVE_DIFFSERV
  nsm_server_set_service (ns, NSM_SERVICE_DIFFSERV);
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_VPLS
  nsm_server_set_service (ns, NSM_SERVICE_VPLS);
#endif /* HAVE_VPLS */
#ifdef HAVE_BGP_VPLS
  nsm_server_set_service (ns, NSM_SERVICE_BGP_VPLS);
#endif /* HAVE_BGP_VPLS */
#ifdef HAVE_DSTE
  nsm_server_set_service (ns, NSM_SERVICE_DSTE);
#endif /* HAVE_DSTE */
#ifdef HAVE_L2
  nsm_server_set_service (ns, NSM_SERVICE_BRIDGE);
#ifdef HAVE_VLAN
  nsm_server_set_service (ns, NSM_SERVICE_VLAN);
#endif /* HAVE_VLAN */

#endif /* HAVE_L2 */

  /* Set callback functions to NSM.  */
  nsm_server_set_callback (ns, NSM_MSG_SERVICE_REQUEST,
                           nsm_parse_service,
                           nsm_server_recv_service);
#ifdef HAVE_L3
  nsm_server_set_callback (ns, NSM_MSG_REDISTRIBUTE_SET,
                           nsm_parse_redistribute,
                           nsm_server_recv_redistribute_set);
  nsm_server_set_callback (ns, NSM_MSG_REDISTRIBUTE_UNSET,
                           nsm_parse_redistribute,
                           nsm_server_recv_redistribute_unset);
  nsm_server_set_callback (ns, NSM_MSG_ISIS_WAIT_BGP_SET,
                           nsm_parse_bgp_conv,
                           nsm_server_recv_isis_wait_bgp_set);
  nsm_server_set_callback (ns, NSM_MSG_BGP_CONV_DONE,
                           nsm_parse_bgp_conv,
                           nsm_server_recv_bgp_conv_done);
  nsm_server_set_callback (ns, NSM_MSG_BGP_CONF_UPDATE,
                           nsm_parse_bgp_conf_update,
                           nsm_server_recv_bgp_conf_update);
  nsm_server_set_callback (ns, NSM_MSG_LDP_SESSION_STATE,
                           nsm_parse_ldp_session_state,
                           nsm_server_recv_ldp_session_state);
  nsm_server_set_callback (ns, NSM_MSG_LDP_SESSION_QUERY,
                           nsm_parse_ldp_session_state,
                           nsm_server_recv_ldp_session_query);
#endif /* HAVE_L3 */

#ifdef HAVE_CRX
  nsm_server_set_callback (ns, NSM_MSG_ADDR_ADD, nsm_parse_address,
                           nsm_server_recv_interface_addr_add);
  nsm_server_set_callback (ns, NSM_MSG_ADDR_DELETE, nsm_parse_address,
                           nsm_server_recv_interface_addr_del);
#endif /* HAVE_CRX */

#ifdef HAVE_L3
  nsm_rib_set_server_callback (ns);
#endif

#ifdef HAVE_CRX
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_CLEAN,
                           nsm_parse_route_clean,
                           nsm_server_recv_route_clean);
  nsm_server_set_callback (ns, NSM_MSG_PROTOCOL_RESTART,
                           nsm_parse_protocol_restart,
                           nsm_server_recv_protocol_restart);
#endif /* HAVE_CRX */

#ifdef HAVE_L3
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_PRESERVE,
                           nsm_parse_route_manage,
                           nsm_server_recv_route_preserve);
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_STALE_REMOVE,
                           nsm_parse_route_manage,
                           nsm_server_recv_route_stale_remove);
#ifdef HAVE_RESTART
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_STALE_MARK,
                           nsm_parse_route_manage,
                           nsm_server_recv_route_stale_mark);
#endif /* HAVE_RESTART */

#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      nsm_server_set_callback (ns, NSM_MSG_LABEL_POOL_REQUEST,
                               nsm_parse_label_pool,
                               nsm_server_recv_label_pool_request);
      nsm_server_set_callback (ns, NSM_MSG_LABEL_POOL_RELEASE,
                               nsm_parse_label_pool,
                               nsm_server_recv_label_pool_release);
#ifdef HAVE_PACKET
      nsm_server_set_callback (ns, NSM_MSG_ILM_LOOKUP,
                               nsm_parse_ilm_lookup,
                               nsm_server_recv_ilm_lookup);
      nsm_server_set_callback (ns, NSM_MSG_FTN_LOOKUP_BY_MPLS_OWNER,
                               nsm_parse_ftn_lookup_by_owner,
                               nsm_server_recv_ftn_lookup_by_owner);
#endif /* HAVE_PACKET */
    }
#endif /* HAVE_MPLS */
#ifdef HAVE_TE
#ifdef HAVE_MPLS
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_INIT,
                           nsm_parse_qos_client_init,
                           nsm_read_qos_client_init);
#endif
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_PROBE,
                           nsm_parse_qos,
                           nsm_read_qos_client_probe);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_RESERVE,
                           nsm_parse_qos,
                           nsm_read_qos_client_reserve);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_MODIFY,
                           nsm_parse_qos,
                           nsm_read_qos_client_modify);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_RELEASE,
                           nsm_parse_qos_release,
                           nsm_read_qos_client_release);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_CLEAN,
                           nsm_parse_qos_clean,
                           nsm_read_qos_client_clean);
#ifdef HAVE_GMPLS
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_INIT,
                           nsm_parse_gmpls_qos_client_init,
                           nsm_read_gmpls_qos_client_init);

  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_PROBE,
                           nsm_parse_gmpls_qos,
                           nsm_read_gmpls_qos_client_probe);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_RESERVE,
                           nsm_parse_gmpls_qos,
                           nsm_read_gmpls_qos_client_reserve);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_MODIFY,
                           nsm_parse_gmpls_qos,
                           nsm_read_gmpls_qos_client_modify);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_RELEASE,
                           nsm_parse_gmpls_qos_release,
                           nsm_read_gmpls_qos_client_release);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_CLEAN,
                           nsm_parse_gmpls_qos_clean,
                           nsm_read_gmpls_qos_client_clean);
#endif /* HAVE_GMPLS */
#endif /* HAVE_TE */
#ifdef HAVE_GMPLS
  nsm_server_set_callback (ns, NSM_MSG_ILM_GEN_LOOKUP,
                               nsm_parse_ilm_gen_lookup,
                               nsm_server_recv_ilm_gen_lookup);

  nsm_server_set_callback (ns, NSM_MSG_DATA_LINK,
                           nsm_parse_data_link,
                           nsm_read_data_link);

  nsm_server_set_callback (ns, NSM_MSG_DATA_LINK_SUB,
                           nsm_parse_data_link_sub,
                           nsm_read_data_link_sub);

  nsm_server_set_callback (ns, NSM_MSG_TE_LINK,
                           nsm_parse_te_link,
                           nsm_read_te_link);

  nsm_server_set_callback (ns, NSM_MSG_GENERIC_LABEL_POOL_REQUEST,
                               nsm_parse_generic_label_pool,
                               nsm_server_recv_gen_label_pool_request);
  nsm_server_set_callback (ns, NSM_MSG_GENERIC_LABEL_POOL_RELEASE,
                               nsm_parse_generic_label_pool,
                               nsm_server_recv_gen_label_pool_release);
  nsm_server_set_callback (ns, NSM_MSG_GENERIC_LABEL_POOL_IN_USE,
                               nsm_parse_generic_label_pool,
                               nsm_server_recv_gen_label_pool_validate);

  nsm_server_set_callback (ns, NSM_MSG_CONTROL_CHANNEL,
                               nsm_parse_control_channel,
                               nsm_server_recv_control_channel);

  nsm_server_set_callback (ns, NSM_MSG_CONTROL_ADJ,
                               nsm_parse_control_adj,
                               nsm_server_recv_control_adj);

#endif /* HAVE_GMPLS */
#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
#ifdef HAVE_PACKET
      nsm_server_set_callback (ns, NSM_MSG_MPLS_FTN_IPV4,
                               nsm_parse_ftn_ipv4,
                               nsm_server_recv_ftn_ipv4);
#ifdef HAVE_IPV6
      nsm_server_set_callback (ns, NSM_MSG_MPLS_FTN_IPV6,
                               nsm_parse_ftn_ipv6,
                               nsm_server_recv_ftn_ipv6);
#endif
      nsm_server_set_callback (ns, NSM_MSG_MPLS_ILM_IPV4,
                               nsm_parse_ilm_ipv4,
                               nsm_server_recv_ilm_ipv4);
#ifdef HAVE_IPV6
      nsm_server_set_callback (ns, NSM_MSG_MPLS_ILM_IPV6,
                               nsm_parse_ilm_ipv6,
                               nsm_server_recv_ilm_ipv6);
#endif /* HAVE_IPV6 */
#endif /* HAVE_PACKET */
      nsm_server_set_callback (ns, NSM_MSG_MPLS_FTN_GEN_IPV4,
                               nsm_parse_ftn_gen_data,
                               nsm_server_recv_ftn_gen_ipv4);

      nsm_server_set_callback (ns, NSM_MSG_MPLS_ILM_GEN_IPV4,
                               nsm_parse_ilm_gen_data,
                               nsm_server_recv_ilm_gen_ipv4);

      nsm_server_set_callback (ns, NSM_MSG_MPLS_IGP_SHORTCUT_ROUTE,
                               nsm_parse_igp_shortcut_route,
                               nsm_server_recv_igp_shortcut_route);
#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
      nsm_server_set_callback (ns, NSM_MSG_MPLS_OAM_L3VPN,
                               nsm_parse_mpls_vpn_rd,
                               nsm_server_recv_mpls_vpn_rd);
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_BIDIR_FTN,
                           nsm_parse_gmpls_ftn_bidir_data,
                           nsm_server_recv_gmpls_bidir_ftn);

  nsm_server_set_callback (ns, NSM_MSG_GMPLS_BIDIR_ILM,
                           nsm_parse_gmpls_ilm_bidir_data,
                           nsm_server_recv_gmpls_bidir_ilm);
#endif /*HAVE_GMPLS*/

#ifdef HAVE_MPLS_VC
  nsm_server_set_callback (ns, NSM_MSG_MPLS_VC_FIB_ADD,
                           nsm_parse_vc_fib_add,
                           nsm_server_recv_vc_fib_add);
  nsm_server_set_callback (ns, NSM_MSG_MPLS_VC_FIB_DELETE,
                           nsm_parse_vc_fib_delete,
                           nsm_server_recv_vc_fib_del);
  nsm_server_set_callback (ns, NSM_MSG_MPLS_VC_TUNNEL_INFO,
                           nsm_parse_vc_tunnel_info,
                           nsm_server_recv_tunnel_info);
  nsm_server_set_callback (ns, NSM_MSG_MPLS_PW_STATUS,
                           nsm_parse_pw_status,
                           nsm_server_recv_pw_status);
#endif /* HAVE_MPLS_VC */

#ifndef HAVE_IMI
  host_user_add_callback (zg, USER_CALLBACK_UPDATE,
                          nsm_server_user_update_callback);
  host_user_add_callback (zg, USER_CALLBACK_DELETE,
                          nsm_server_user_delete_callback);
#endif /* HAVE_IMI */

  nsm_server_set_callback (ns, NSM_MSG_IF_DEL_DONE,
                           nsm_parse_link,
                           nsm_server_recv_if_del_done);

#ifdef HAVE_RMOND
  nsm_server_set_callback (ns, NSM_MSG_RMON_REQ_STATS,
                           nsm_parse_rmon_msg,
                           nsm_server_recv_rmon_req_statistics);
#endif /* HAVE_RMON */

#ifdef HAVE_MPLS_FRR
  nsm_server_set_callback (ns, NSM_MSG_RSVP_CONTROL_PACKET,
                           nsm_parse_rsvp_control_packet,
                           nsm_server_recv_rsvp_control_packet);
#endif /* HAVE_MPLS_FRR */
#ifdef HAVE_NSM_MPLS_OAM
  nsm_server_set_callback (ns, NSM_MSG_MPLS_ECHO_REQUEST,
                           nsm_parse_mpls_oam_request,
                           nsm_server_recv_mpls_oam_req);
#elif defined (HAVE_MPLS_OAM)
/* Set parser & callback functions for echo request/reply process
 * messages from OAMD.
 */
  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REQ_PROCESS,
                           nsm_parse_mpls_oam_req_process,
                           nsm_server_recv_mpls_oam_req_process);

  /* Added to handle icmp request/reply */
  nsm_server_set_callback (ns, NSM_MSG_OAM_MPLS_ICMP_PING_REQ_PROCESS,
                           nsm_parse_mpls_oam_icmp_req_process,
                           nsm_server_recv_mpls_oam_icmp_req_process);

  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_LDP,
                           nsm_parse_mpls_oam_rep_process_ldp,
                           nsm_server_recv_mpls_oam_rep_process_ldp);

  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_GEN,
                           nsm_parse_mpls_oam_rep_process_gen,
                           nsm_server_recv_mpls_oam_rep_process_gen);

  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_RSVP,
                           nsm_parse_mpls_oam_rep_process_rsvp,
                           nsm_server_recv_mpls_oam_rep_process_rsvp);

#ifdef HAVE_MPLS_VC
  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_L2VC,
                           nsm_parse_mpls_oam_rep_process_l2vc,
                           nsm_server_recv_mpls_oam_rep_process_l2vc);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_L3VPN,
                           nsm_parse_mpls_oam_rep_process_l3vpn,
                           nsm_server_recv_mpls_oam_rep_process_l3vpn);
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS_TP
  nsm_server_set_callback (ns, NSM_MSG_MPLS_TP_PATH_REQUEST,
                           nsm_parse_mpls_tp_oam_request,
                           nsm_server_recv_mpls_tp_path_request);

#ifdef HAVE_IETF_MPLS_TP_OAM
  nsm_server_set_callback (ns, NSM_MSG_MPLS_TP_LPS_MESSAGE,
                           nsm_parse_mpls_tp_aps_info,
                           nsm_server_recv_mpls_tp_aps_info);
#endif /* HAVE_IETF_MPLS_TP_OAM */
#endif /* HAVE_MPLS_TP */
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_MPLS_OAM_ITUT
  nsm_server_set_callback (ns, NSM_MSG_MPLS_OAM_ITUT_PROCESS_REQ,
                           nsm_parse_mpls_oam_process_request,
                           nsm_server_recv_mpls_oam_process_req);
#endif /* HAVE_MPLS_OAM_ITUT */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_L2
  nsm_bridge_set_server_callback (ns);
#endif /* HAVE_L2 */

#ifdef HAVE_VLAN
  nsm_vlan_set_server_callback (ns);
#endif /* HAVE_VLAN */

#ifdef HAVE_AUTHD
  nsm_auth_set_server_callback (ns);

#endif /* HAVE_AUTHD */

#ifdef HAVE_LACP
  nsm_lacp_set_server_callback (ns);
#endif /* HAVE_LACP */

#ifdef HAVE_ONMD
  nsm_oam_set_server_callback (ns);
#endif /* HAVE_ONMD */

#ifdef HAVE_ELMID
  nsm_elmi_set_server_callback (ns);
#endif /* HAVE_ELMID  */

#if defined (HAVE_RSTPD) || defined (HAVE_STPD) || defined (HAVE_MSTPD) || defined (HAVE_RPVST_PLUS)
  nsm_stp_set_server_callback (ns);
#endif /* (HAVE_RSTPD) || (HAVE_STPD) || ((HAVE_MSTPD) || HAVE_RPVST_PLUS */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_TRILLD
  nsm_trill_set_server_callback (ns);
#endif /* HAVE_TRILLD */
#ifdef HAVE_SPBD
  nsm_spb_set_server_callback (ns);
#endif //HAVE_SPBD


  ng->server = ns;

  return ns;

#endif/*0*/
}

int32_t
cpm_server_finish (struct cpm_server *cs, void *cs_appln)
{
  struct cpm_server_client *csc;
  struct cpm_server_entry *cse, *next;
  int32_t i;

  /* Clean up the CPM server clients.  */
  for (i = 0; i < vector_max (cs->client); i++)
    if ((csc = vector_slot (cs->client, i)) != NULL)
      {
        for (cse = csc->head; cse; cse = next)
          {
            next = cse->next;
            message_entry_free (cse->me);

            /* Turn off the write thread */
            THREAD_OFF (cse->t_write);

            cpm_server_entry_free (cse);
          }
        XFREE (MTYPE_CPM_SERVER_CLIENT, csc);
      }
  vector_free (cs->client);

  message_server_stop (cs->ms);
  message_server_delete (cs->ms);

  XFREE (MTYPE_CPM_SERVER, cs_appln);

  return 0;
}
