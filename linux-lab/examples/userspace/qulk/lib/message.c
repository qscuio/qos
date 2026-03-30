/* Generic message manager library.  This library includes both client
   and server side code.  */

#include "lib.h"
#include "xthread.h"
#include "vector.h"
#include "message.h"
#include "vlog.h"
#include "tlv.h"

VLOG_DEFINE_THIS_MODULE(xmessage);

DEFINE_MGROUP(XMESSAGE, "Program for test memory api");
DEFINE_MTYPE_STATIC(XMESSAGE, MESSAGE_ENTRY,  "Message Entry");
DEFINE_MTYPE_STATIC(XMESSAGE, MESSAGE_HANDLER, "Message Handler");

int message_server_read (struct thread *);

/* Message client entry memory allocation.  */
struct message_entry *
message_entry_new ()
{
  struct message_entry *me;

  me = XCALLOC (MTYPE_MESSAGE_ENTRY, sizeof (struct message_entry));

  return me;
}

/* Message client entry memory free.  */
void
message_entry_free (struct message_entry *me)
{
  THREAD_OFF (me->t_read);

  if (me->sock >= 0)
    close (me->sock);

  XFREE (MTYPE_MESSAGE_ENTRY, me);
}

/* Client is connected to server.  After accept socket, read thread is
   created to read client packet.  */
struct message_entry *
message_entry_register (struct message_handler *ms, sock_handle_t sock)
{
  struct message_entry *me;

  me = message_entry_new ();
  me->sock = sock;
  me->zg = ms->zg;
  me->ms = ms;
  thread_add_read (ms->zg->master, message_server_read, me, sock, &me->t_read);

  return me;
}

/* Create message server.  */
struct message_handler *
message_server_create (struct abc *zg)
{
  struct message_handler *ms;

  ms = XCALLOC (MTYPE_MESSAGE_HANDLER, sizeof (struct message_handler));
  ms->zg = zg;
  ms->clist = vector_init (1);
  ms->sock = -1; /* initialize to -1, s >= 0 is valid socket value */
  ms->remote_conn = 0;

  return ms;
}

int
message_server_delete (struct message_handler *ms)
{
  if (ms->path)
    XFREE (MTYPE_TMP, ms->path);

  vector_free (ms->clist);

  THREAD_OFF (ms->t_read);
  THREAD_OFF (ms->t_connect);

  XFREE (MTYPE_MESSAGE_HANDLER, ms);

  return 0;
}

#ifndef HAVE_TCP_MESSAGE
/* Set message server type to UNIX domain.  Argument path is a path
   for UNIX domain socket server.  */
void
message_server_set_style_domain (struct message_handler *ms, char *path)
{
  message_server_stop (ms);
  ms->style = MESSAGE_STYLE_UNIX_DOMAIN;
  if (ms->path)
    XFREE (MTYPE_TMP, ms->path);
  ms->path = XSTRDUP (MTYPE_TMP, path);
}
#endif /* HAVE_TCP_MESSAGE */

/* Set message server type to TCP.  */
void
message_server_set_style_tcp (struct message_handler *ms, u_int16_t port)
{
  message_server_stop (ms);
  ms->style = MESSAGE_STYLE_TCP;
  ms->port = port;
}

/* Set event callback function.  */
void
message_server_set_callback (struct message_handler *ms, int event,
                             MESSAGE_CALLBACK cb)
{
  if (event >= MESSAGE_EVENT_MAX)
    return;

  ms->callback[event] = cb;
}

/* Message client disconnected.  */
void
message_server_disconnect (struct message_handler *ms,
                           struct message_entry *me, sock_handle_t sock)
{
  /* Cancel read thread.  */
  THREAD_OFF (me->t_read);

#ifdef HAVE_CMLD
  int ret = RESULT_ERROR;
  /* For cmld (where we have multiple threads),
     need to check if we can free now or not */
  if (ms->callback[MESSAGE_EVENT_DISCONNECT_VALIDATE])
  {
    ret = (*ms->callback[MESSAGE_EVENT_DISCONNECT_VALIDATE]) (ms, me, sock);
    if (ret == RESULT_ERROR)
    {
      /* Return for now, Will free later */
      return;
    }
  }
#endif /* HAVE_CMLD */

  /* Close socket.  */
  close (sock);
  me->sock = -1;

  /* Call callback function if registered.  */
  if (ms->callback[MESSAGE_EVENT_DISCONNECT])
    (*ms->callback[MESSAGE_EVENT_DISCONNECT]) (ms, me, sock);

  message_entry_free (me);
}

/* Message server read thread.  */
int
message_server_read (struct thread *t)
{
  struct message_entry *me;
  struct message_handler *ms;
  sock_handle_t sock;
  int nbytes = 0;

  sock = THREAD_FD (t);
  me = THREAD_ARG (t);
  me->t_read = NULL;
  ms = me->ms;

  /* Call read header callback function.  */
  if (ms->callback[MESSAGE_EVENT_READ_HEADER])
    {
      nbytes = (*ms->callback[MESSAGE_EVENT_READ_HEADER]) (ms, me, sock);

      /* Disconnection event handler.  */
      if (nbytes < 0)
        {
          message_server_disconnect (ms, me, sock);
          return -1;
        }
    }

  /* Call read message body callback function.  */
  if (ms->callback[MESSAGE_EVENT_READ_MESSAGE])
    {
      nbytes = (*ms->callback[MESSAGE_EVENT_READ_MESSAGE]) (ms, me, sock);

      /* Disconnection event handler.  */
      if (nbytes < 0)
        {
          message_server_disconnect (ms, me, sock);
          return -1;
        }
    }

  /* Re-register read thread.  */
  thread_add_read (ms->zg->master, message_server_read, me, sock, &me->t_read);

  return 0;
}

/* Reregister server read thread.  */
void
message_server_read_reregister (struct message_entry *me)
{
  struct message_handler *ms;

  ms = me->ms;

  if (me->t_read)
    thread_cancel (&me->t_read);

  thread_add_read (ms->zg->master, message_server_read, me, me->sock, &me->t_read);
}

int
message_server_rcvbuf (struct message_entry *me, int32_t size)
{
  return set_recv_buffer (me->sock, size);
}

int
message_server_sndbuf (struct message_entry *me, int32_t size)
{
  return set_send_buffer (me->sock, size);
}

/* Accept client connection.  */
int
message_server_accept (struct thread *t)
{
  struct message_handler *ms;
  struct message_entry *me;
  sock_handle_t sock;
  sock_handle_t csock;
#ifdef HAVE_TCP_MESSAGE
  struct sockaddr_in addr;
#else /* HAVE_TCP_MESSAGE */
  struct sockaddr_un addr;
#endif /* HAVE_TCP_MESSAGE */
  socklen_t len;
  int ret;

  ms = THREAD_ARG (t);
  ms->t_read = NULL;
  sock = THREAD_FD (t);
  len = sizeof (addr);

  VLOG_INFO("Received connect message !!!!\n");
  /* Re-register accept thread.  */
  thread_add_read (ms->zg->master, message_server_accept, ms, sock, &ms->t_read);

  /* Accept and get client socket.  */
  csock = accept (sock, (struct sockaddr *) &addr, &len);
  if (csock < 0)
    {
#if 0 /* @liwei */
      zlog_warn (ms->zg, "accept socket error: %s", strerror (errno));
#endif
      return -1;
    }

  /* Make socket non-blocking.  */
  set_nonblocking (csock);

  /* Register client to client list.  */
  me = message_entry_register (ms, csock);

  VLOG_INFO("Received connect message !!!!\n");
  /* Call back function. */
  if (ms->callback[MESSAGE_EVENT_CONNECT])
    {
      ret = (*ms->callback[MESSAGE_EVENT_CONNECT]) (ms, me, csock);
      if (ret < 0)
        message_entry_free (me);
    }
  else
    message_entry_free (me);

  return 0;
}

#ifndef HAVE_TCP_MESSAGE
sock_handle_t
message_server_socket_unix (struct message_handler *mh)
{
  sock_handle_t sock;
  struct sockaddr_un addr;
  int len;
#if defined (HAVE_DS_HSL_ASYNC_SOCKET_BUF_SIZE)
  int size = 0;
#endif /* HAVE_DS_HSL_ASYNC_SOCKET_BUF_SIZE */

  /* Create UNIX domain socket.  */
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    {
#if 0
      zlog_warn (mh->zg, "socket create error: %s", strerror (errno));
#endif
      return -1;
    }

  /* Unlink. */
  unlink (mh->path);

  /* Prepare accept socket. */
  memset (&addr, 0, sizeof (struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, mh->path, strlen (mh->path));
  addr.sun_path[strlen(mh->path)] = '\0';
#ifdef HAVE_SUN_LEN
  len = addr.sun_len = SUN_LEN (&addr);
#else
  len = sizeof (addr.sun_family) + strlen (addr.sun_path);
#endif /* HAVE_SUN_LEN */

  /* Bind socket. */
  if (bind (sock, (struct sockaddr *) &addr, len) < 0)
    {
#if 0
      zlog_warn (mh->zg, "bind socket error: %s", strerror (errno));
#endif
      close (sock);
      return -1;
    }

  return sock;
}
#endif /* HAVE_TCP_MESSAGE */

sock_handle_t
message_server_socket_tcp (struct message_handler *mh)
{
  sock_handle_t sock;
  struct sockaddr_in addr;
  int len = sizeof (struct sockaddr_in);
  int state = 1; /* on. */

  /* Create TCP socket.  */
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
#if 0
      zlog_warn (mh->zg, "socket create error: %s", strerror (errno));
#endif
      return -1;
    }

  /* Prepare accept socket.  */
  memset (&addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
  addr.sin_len = len;
#endif /* HAVE_SIN_LEN */
  addr.sin_port = htons (mh->port);

  if (mh->remote_conn == 1)
  {
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
  }
  else
  {
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  }

  set_reuse_addr (sock, state);
  set_reuse_port (sock, state);

  /* Bind socket. */
  if (bind (sock, (struct sockaddr *) &addr, len) < 0)
    {
#if 0
      zlog_warn (mh->zg, "bind socket error: %s", strerror (errno));
#endif
      close (sock);
      return -1;
    }

  return sock;
}

/* Create server socket according to specified server type.  Then wait
   a connection from client.  */
int
message_server_start (struct message_handler *ms)
{
#ifdef HAVE_TCP_MESSAGE
  ms->sock = message_server_socket_tcp (ms);
#else /* HAVE_TCP_MESSAGE */
  if (ms->style == MESSAGE_STYLE_UNIX_DOMAIN)
    ms->sock = message_server_socket_unix (ms);
  else if (ms->style == MESSAGE_STYLE_TCP)
    ms->sock = message_server_socket_tcp (ms);
  else
    return -1;
#endif /* HAVE_TCP_MESSAGE */

  if (ms->sock == -1)
    return -1;

  /* Listen to the socket.  */
  /* Increased maximum simulataneous connections to be received by accept socket */
  if (listen (ms->sock, 30) < 0)
    {
#if 0
      zlog_warn (ms->zg, "listen socket error: %s", strerror (errno));
#endif
      close (ms->sock);
      ms->sock = -1;
      return -1;
    }

  VLOG_INFO("Start to accept\n");
  /* Register read thread.  */
  thread_add_read (ms->zg->master, message_server_accept, ms, ms->sock, &ms->t_read);

  return 0;
}

/* Stop message server.  */
int
message_server_stop (struct message_handler *ms)
{
  if (ms->sock >= 0)
    {
      close (ms->sock);
      ms->sock = -1;
    }
  return 0;
}

int
message_client_rcvbuf (struct message_handler *mc, int32_t size)
{
  return set_recv_buffer (mc->sock, size);
}

int
message_client_sndbuf (struct message_handler *mc, int32_t size)
{
  return set_send_buffer (mc->sock, size);
}

/* Create client with spcified type.  Type must be one of
   MESSAGE_TYPE_ASYNC or MESSAGE_TYPE_SYNC.  */
struct message_handler *
message_client_create (struct abc *zg, int type)
{
  struct message_handler *mc;

  mc = XCALLOC (MTYPE_MESSAGE_HANDLER, sizeof (struct message_handler));
  mc->zg = zg;
  mc->type = type;
  mc->sock = -1;
  mc->remote_conn = 0;

  return mc;
}

/* Stop message client.  */
int
message_client_stop (struct message_handler *mc)
{
  if (mc->sock >= 0)
    {
      close (mc->sock);
      mc->sock = -1;
    }

  THREAD_OFF (mc->t_read);
  THREAD_OFF (mc->t_connect);

  return 0;
}

int
message_client_delete (struct message_handler *mc)
{
  /* Stop the client.  */
  message_client_stop (mc);

  if (mc->path)
    XFREE (MTYPE_TMP, mc->path);

  if (mc->clist)
    vector_only_wrapper_free (mc->clist);

  THREAD_OFF (mc->t_read);
  THREAD_OFF (mc->t_connect);

  XFREE (MTYPE_MESSAGE_HANDLER, mc);

  return 0;
}

#ifndef HAVE_TCP_MESSAGE
/* Set message client type to UNIX domain.  Argument path is a path
   for UNIX domain socket server.  */
void
message_client_set_style_domain (struct message_handler *mc, char *path)
{
  message_client_stop (mc);
  mc->style = MESSAGE_STYLE_UNIX_DOMAIN;
  if (mc->path)
    XFREE (MTYPE_TMP, mc->path);
  mc->path = XSTRDUP (MTYPE_TMP, path);
}
#endif /* !HAVE_TCP_MESSAGE */

/* Set message server type to TCP.  */
void
message_client_set_style_tcp (struct message_handler *mc, u_int16_t port)
{
  message_client_stop (mc);
  mc->style = MESSAGE_STYLE_TCP;
  mc->port = port;
}

/* Set event callback function.  */
void
message_client_set_callback (struct message_handler *ms, int event,
                             MESSAGE_CALLBACK cb)
{
  if (event >= MESSAGE_EVENT_MAX)
    return;

  ms->callback[event] = cb;
}

/* Message client disconnected.  */
void
message_client_disconnect (struct message_handler *mc, sock_handle_t sock)
{
  /* Stop events.  */
  message_client_stop (mc);

  /* Call callback function if registered.  */
  if (mc->callback[MESSAGE_EVENT_DISCONNECT])
    (*mc->callback[MESSAGE_EVENT_DISCONNECT]) (mc, NULL, -1);
}

/* Message server read thread.  */
int
message_client_read (struct thread *t)
{
  struct message_handler *mc;
  sock_handle_t sock;
  int nbytes = 0;

  sock = THREAD_FD (t);
  mc = THREAD_ARG (t);
  mc->t_read = NULL;

  /* Call read message body callback function.  */
  if (mc->callback[MESSAGE_EVENT_READ_MESSAGE])
    {
      nbytes = (*mc->callback[MESSAGE_EVENT_READ_MESSAGE]) (mc, NULL, sock);

      /* Disconnection event handler.  */
      if (nbytes < 0)
        {
          message_client_disconnect (mc, sock);
          return -1;
        }
    }

  /* Re-register read thread.  */
  if (mc->type == MESSAGE_TYPE_ASYNC)
    thread_add_read (mc->zg->master, message_client_read, mc, mc->sock, &mc->t_read);
  else if (mc->type == MESSAGE_TYPE_ASYNC_HIGH)
    thread_add_read (mc->zg->master, message_client_read, mc, mc->sock, &mc->t_read);
  else if (mc->type == MESSAGE_TYPE_ASYNC_ET)
    thread_add_read (mc->zg->master, message_client_read, mc, mc->sock, &mc->t_read);

  return 0;
}

/* Register read thread.  */
void
message_client_read_register (struct message_handler *mc)
{
  if (mc->t_read)
    thread_cancel (&mc->t_read);

  if (mc->type == MESSAGE_TYPE_ASYNC)
    thread_add_read (mc->zg->master, message_client_read, mc, mc->sock, &mc->t_read);
  else if (mc->type == MESSAGE_TYPE_ASYNC_HIGH)
    thread_add_read (mc->zg->master, message_client_read, mc, mc->sock, &mc->t_read);
  else if (mc->type == MESSAGE_TYPE_ASYNC_ET)
    thread_add_read (mc->zg->master, message_client_read, mc, mc->sock, &mc->t_read);
}

/* Re-register read thread.  */
void
message_client_read_reregister (struct message_handler *mc)
{
  if (mc->t_read)
    {
      thread_cancel (&mc->t_read);
      thread_add_read (mc->zg->master, message_client_read, mc, mc->sock, &mc->t_read);
    }
}

sock_handle_t
message_client_socket_unix (struct message_handler *mh)
{
  sock_handle_t sock;
  struct sockaddr_un addr;
  int len;
#if defined (HAVE_DS_HSL_ASYNC_SOCKET_BUF_SIZE)
  int size = 0;
#endif /* HAVE_DS_HSL_ASYNC_SOCKET_BUF_SIZE */

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    return -1;

  /* Prepare client UNIX domain connection. */
  memset (&addr, 0, sizeof (struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, mh->path, strlen (mh->path));
#ifdef HAVE_SUN_LEN
  len = addr.sun_len = SUN_LEN (&addr);
#else
  len = sizeof (addr.sun_family) + strlen (addr.sun_path);
#endif /* HAVE_SUN_LEN */

  /* Connect to the server. */
  if (connect (sock, (struct sockaddr *) &addr, len) < 0)
    {
      close (sock);
      return -1;
    }

  return sock;
}

sock_handle_t
message_client_socket_tcp (struct message_handler *mh)
{
  sock_handle_t sock;
  struct sockaddr_in addr;
  int len = sizeof (struct sockaddr_in);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return -1;

  /* Prepare client TCP connection. */
  memset (&addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (mh->port);
  if (mh->remote_conn == 1)
    {
      addr.sin_addr.s_addr = mh->remote_addr.sin_addr.s_addr ;
    }
  else
    {
      addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    }

#ifdef HAVE_SIN_LEN
  addr.sin_len = len;
#endif /* HAVE_SIN_LEN */

  /* Connect to the server. */
  if (connect (sock, (struct sockaddr *) &addr, len) < 0)
    {
      close (sock);
      return -1;
    }

  return sock;
}

int
message_client_start (struct message_handler *mc)
{
  /* If already connected to NSM. */
  if (mc->sock != -1)
    return 0;

  /* Check connect thread. */
  if (mc->t_connect)
    return 0;

  if (mc->style == MESSAGE_STYLE_UNIX_DOMAIN)
    mc->sock = message_client_socket_unix (mc);
  else if (mc->style == MESSAGE_STYLE_TCP)
    mc->sock = message_client_socket_tcp (mc);
  else
    return -1;

  if (mc->sock == -1)
    return -1;

  /* Connection established.
     Read thread is launched by connect handler. */
  if (mc->callback[MESSAGE_EVENT_CONNECT])
    (*mc->callback[MESSAGE_EVENT_CONNECT]) (mc, NULL, mc->sock);

  return mc->sock;
}



void
message_queue_init (struct abc *zg,
                    struct message_queue *mq)
{
  mq->zg = zg;
  FIFO_INIT (&mq->fifo);
}

struct message_queue_entry *
message_queue_top (struct message_queue *mq)
{
  return (struct message_queue_entry *)FIFO_TOP (&mq->fifo);
}

void
message_queue_clear (struct message_queue *mq)
{
  struct message_queue_entry *mqe;

  while ((mqe = (struct message_queue_entry *)FIFO_TOP (&mq->fifo)))
    {
      FIFO_DEL (mqe);
      XFREE (MTYPE_TMP, mqe->buf);
      XFREE (MTYPE_TMP, mqe);
    }
  THREAD_OFF (mq->t_write);
}

/* De-queue NSM message.  */
int
message_queue_entry_get (struct thread *t)
{
  struct message_queue *mq = THREAD_ARG (t);
  struct message_queue_entry *mqe;
  sock_handle_t sock = THREAD_FD (t);
  int nbytes;

  mq->t_write = NULL;

  mqe = (struct message_queue_entry *) FIFO_HEAD (&mq->fifo);
  if (mqe)
    {
      nbytes = write (sock, mqe->buf + mqe->written,
                               mqe->length - mqe->written);
      if (nbytes <= 0)
        {
          if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
#if 0
              zlog_err (mq->zg, "NSM message send error socket %d %s",
                        sock, strerror (errno));
#endif
              return -1;
            }
        }
      else if (nbytes != (mqe->length - mqe->written))
        mqe->written += nbytes;
      else
        {
          FIFO_DEL (mqe);
          XFREE (MTYPE_TMP, mqe->buf);
          XFREE (MTYPE_TMP, mqe);
        }
    }

  if (FIFO_TOP (&mq->fifo))
    thread_add_write(mq->zg->master, message_queue_entry_get, mq, sock, &mq->t_write);

  return 0;
}

void
message_queue_entry_set (struct message_queue *mq,
                         sock_handle_t sock, u_char *buf,
                         u_int16_t length, u_int16_t written)
{
  struct message_queue_entry *mqe;

  mqe = XCALLOC (MTYPE_TMP, sizeof (struct message_queue_entry));
  mqe->buf = XMALLOC (MTYPE_TMP, length);
  memcpy (mqe->buf, buf, length);
  mqe->length = length;
  mqe->written = written;

  FIFO_ADD (&mq->fifo, mqe);

  thread_add_write (mq->zg->master, message_queue_entry_get, mq, sock, &mq->t_write);
}

/* Protocol message header encode.  */
int
message_header_encode (u_char **pnt, u_int16_t *size, struct sockaddr_l2 *skhdr)
{
  u_char *sp = *pnt;

  if (*size < MESSAGE_HDR_SIZE)
    return EC_LIB_MESSAGE_SIZE_TOO_SMALL;

  TLV_ENCODE_PUT (skhdr->dest_mac, ETHER_ADDR_LEN);
  TLV_ENCODE_PUT (skhdr->src_mac, ETHER_ADDR_LEN);
  TLV_ENCODE_PUTW (skhdr->port);

  return *pnt - sp;
}

/* Protocol message header decode.  */
int
message_header_decode (u_char **pnt, u_int16_t *size, struct sockaddr_l2 *skhdr)
{
  if (*size < MESSAGE_HDR_SIZE)
    return EC_LIB_MESSAGE_SIZE_TOO_SMALL;

  TLV_DECODE_GET (skhdr->dest_mac, ETHER_ADDR_LEN);
  TLV_DECODE_GET (skhdr->src_mac, ETHER_ADDR_LEN);
  TLV_DECODE_GETW (skhdr->port);

  return MESSAGE_HDR_SIZE;
}
