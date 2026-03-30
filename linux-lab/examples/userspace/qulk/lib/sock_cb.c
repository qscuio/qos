/* sock_cb.c,v 1.1.1.1 2012/10/30 17:16:40 bob.macgill Exp */
#include "lib.h"
#include "xthread.h"
#include "linklist.h"
#include "sockunion.h"
#include "cqueue.h"
#include "sock_cb.h"

DEFINE_MGROUP(XMEMORY_SOCK_CB, "Socket Callback");
DEFINE_MTYPE_STATIC(XMEMORY_SOCK_CB, SSOCK_CB,  "Pending Message");

/*
 * NOTE : Following are the Socket-CB API functions
 */

int32_t
stream_sock_cb_zombie_list_alloc (struct abc *zlg)
{
  int32_t ret;

  ret = 0;

  /* Sanity check */
  if (! zlg || SSOCK_CB_GET_ZOMBIE_LIST (zlg))
    {
      ret = -1;
      goto EXIT;
    }

  SSOCK_CB_GET_ZOMBIE_LIST (zlg) = list_new ();

  if (! SSOCK_CB_GET_ZOMBIE_LIST (zlg))
    {
      ret = -1;
      goto EXIT;
    }

EXIT:

  return ret;
}

int32_t
stream_sock_cb_zombie_list_free (struct abc *zlg)
{
  struct stream_sock_cb *ssock_cb;
  struct listnode *next_scb;
  struct list *tmp_list;
  struct listnode *nn;
  int32_t ret;

  ret = 0;
 
  if (! zlg)
    {
      ret = -1;
      goto EXIT;
    }

  tmp_list = SSOCK_CB_GET_ZOMBIE_LIST (zlg);

  /* Sanity check */
  if (! tmp_list)
    {
      ret = -1;
      goto EXIT;
    }

  for (nn = tmp_list->head; nn; nn = next_scb)
    {
      next_scb = nn->next;
      if ((ssock_cb = GETDATA (nn)) != NULL)
        stream_sock_cb_delete (ssock_cb, zlg);
    }
  list_delete (&tmp_list);

  SSOCK_CB_GET_ZOMBIE_LIST (zlg) = NULL;

EXIT:

  return ret;
}

struct stream_sock_cb *
stream_sock_cb_alloc (void *cb_owner,
                      u_int32_t buf_size,
                      ssock_cb_status_func_t status_func,
                                          ssock_cb_tx_ready_func_t tx_ready_func,
                      struct abc *zlg)
{
  struct stream_sock_cb *ssock_cb;
  struct cqueue_buffer *cq_buf;
  struct cqueue_buffer *cq_buf_high;
  int32_t ret;

  ssock_cb = NULL;
  cq_buf = NULL;
  cq_buf_high = NULL;
  ret = 0;

  /* Sanity check */
  if (! cb_owner || ! zlg)
    goto EXIT;

  ssock_cb = XCALLOC (MTYPE_SSOCK_CB, sizeof (struct stream_sock_cb));
  if (! ssock_cb)
    goto EXIT;

  ssock_cb->ssock_fd = -1;
  ssock_cb->ssock_state = SSOCK_STATE_IDLE;
  ssock_cb->ssock_cb_owner = cb_owner;
  ssock_cb->ssock_status_func = status_func;
  ssock_cb->ssock_tx_ready_func = tx_ready_func;
  ssock_cb->ssock_tx_ready_report = 0;
  ssock_cb->ssock_buf_size = buf_size;

  ssock_cb->ssock_ibuf = cqueue_buf_get (buf_size, zlg);
  if (! ssock_cb->ssock_ibuf)
    goto CLEANUP;

#ifdef HAVE_HA
  ret = cqueue_buf_list_alloc (&ssock_cb->ssock_obuf_list, 256, zlg);
#else
  ret = cqueue_buf_list_alloc (&ssock_cb->ssock_obuf_list, ~0, zlg);
#endif /* HAVE_HA */
  if (ret < 0)
    goto CLEANUP;

  cq_buf = cqueue_buf_get (buf_size, zlg);
  if (! cq_buf)
    goto CLEANUP;

  ret = cqueue_buf_listnode_add (ssock_cb->ssock_obuf_list, cq_buf, zlg);
  if (ret < 0)
    goto CLEANUP;

  ret = cqueue_buf_list_alloc (&ssock_cb->ssock_obuf_high_list, ~0, zlg);
  if (ret < 0)
    goto CLEANUP;

  cq_buf_high = cqueue_buf_get (buf_size, zlg);
  if (! cq_buf_high)
    goto CLEANUP;

  ret = cqueue_buf_listnode_add (ssock_cb->ssock_obuf_high_list, cq_buf_high, zlg);
  if (ret < 0)
    goto CLEANUP;

  goto EXIT;

CLEANUP:

  if (cq_buf)
    cqueue_buf_release (cq_buf, zlg);

  if (cq_buf_high)
    cqueue_buf_release (cq_buf_high, zlg);

  if (ssock_cb)
    {
      if (ssock_cb->ssock_ibuf)
        cqueue_buf_release (ssock_cb->ssock_ibuf, zlg);

      if (ssock_cb->ssock_obuf_list)
        cqueue_buf_list_free (ssock_cb->ssock_obuf_list, zlg);

      if (ssock_cb->ssock_obuf_high_list)
        cqueue_buf_list_free (ssock_cb->ssock_obuf_high_list, zlg);

      XFREE (MTYPE_SSOCK_CB, ssock_cb);
      ssock_cb = NULL;
    }

EXIT:

  return ssock_cb;
}

sock_handle_t
stream_sock_cb_get_fd (struct stream_sock_cb *ssock_cb,
                       union sockunion *sck_union,
                       struct abc *zlg,
                       fib_id_t fib_id)
{
  sock_handle_t sck_fd;

  sck_fd = -1;

  /* Sanity check */
  if (! ssock_cb || ! sck_union || ! zlg)
    {
      sck_fd = -1;
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
      if (sck_fd >= 0)
        close (sck_fd);
      sck_fd = socket(sck_union->sa.sa_family, SOCK_STREAM, 0);
      ssock_cb->ssock_fd = sck_fd;
      if (sck_fd >= 0)
        ssock_cb->ssock_state = SSOCK_STATE_ACTIVE;
      break;

    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CONNECTED:
    case SSOCK_STATE_WRITING:
      /* Just return current FD */
      sck_fd = ssock_cb->ssock_fd;
      break;

    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      sck_fd = -1;
      break;
    }

EXIT:

  return sck_fd;
}

void 
stream_sock_set_read (struct stream_sock_cb *ssock_cb,
                      struct abc *zlg)
{
  SSOCK_CB_READ_ON (zlg, ssock_cb->t_ssock_read, ssock_cb,
                    stream_sock_cb_read, ssock_cb->ssock_fd);
}

void 
stream_sock_unset_read (struct stream_sock_cb *ssock_cb,
                      struct abc *zlg)
{
  SSOCK_CB_READ_OFF (zlg, ssock_cb->t_ssock_read);
}

int
stream_sock_connect_repair_mode (struct stream_sock_cb *ssock_cb,
                                 union sockunion *sck_union,
                                 struct abc *zlg)
{
  socklen_t saddr_len;
  int32_t ret;
  saddr_len = 0;
  ret = 0;

  /* Sanity check */
  if (! ssock_cb || ! sck_union || ! zlg)
  {
    ret = -1;
    goto EXIT;
  }

  switch (ssock_cb->ssock_state)
  {
    case SSOCK_STATE_IDLE:
      ret = -1;
      break;

    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CONNECTED:
    case SSOCK_STATE_WRITING:
      switch (sck_union->sa.sa_family)
      {
        case AF_INET:
          saddr_len = sizeof (struct sockaddr_in);
          break;
#ifdef HAVE_IPV6
        case AF_INET6:
          saddr_len = sizeof (struct sockaddr_in6);
          break;
#endif /* AF_INET6 */
      }

      ret = connect (ssock_cb->ssock_fd,
                              (struct sockaddr *) sck_union,
                              saddr_len);
      if (ret < 0)
      {
        ret = -1;
      }
      else if (ssock_cb->ssock_state == SSOCK_STATE_ACTIVE)
      {
        CQUEUE_BUF_RESET (ssock_cb->ssock_ibuf);
        
        ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;
        
        /* Now start the 'read' thread */
        //SSOCK_CB_READ_ON (zlg, ssock_cb->t_ssock_read, ssock_cb,
        //                  stream_sock_cb_read, ssock_cb->ssock_fd);
      }
      else
      {
        ret = -1;
      }

      break;

    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      ret = -1;
      break;
    }
EXIT:
  return ret;
}

int32_t
stream_sock_cb_connect (struct stream_sock_cb *ssock_cb,
                        union sockunion *sck_union,
                        struct abc *zlg)
{
  socklen_t saddr_len;
  int32_t sock_errno;
  int32_t ret;

  sock_errno = 0;
  saddr_len = 0;
  ret = 0;

  /* Sanity check */
  if (! ssock_cb || ! sck_union || ! zlg)
    {
      ret = -1;
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
      ret = -1;
      break;

    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CONNECTED:
    case SSOCK_STATE_WRITING:
      switch (sck_union->sa.sa_family)
        {
        case AF_INET:
          saddr_len = sizeof (struct sockaddr_in);
          break;
#ifdef HAVE_IPV6
        case AF_INET6:
          saddr_len = sizeof (struct sockaddr_in6);
          break;
#endif /* AF_INET6 */
        }

      ret = connect (ssock_cb->ssock_fd,
                              (struct sockaddr *) sck_union,
                              saddr_len);
      if (ret < 0)
        {
          sock_errno = errno;

          switch (sock_errno)
            {
            case EINPROGRESS:
              /*
               * Start 'write' thread so that 'select' wakes us up if
               * Socket status changes (success/failure of 'connect')
               */
              SSOCK_CB_WRITE_ON (zlg, ssock_cb->t_ssock_write,
                                 ssock_cb, stream_sock_cb_write,
                                 ssock_cb->ssock_fd);
              ret = 0;
              break;

            case EALREADY:
            case EISCONN:
              /* Inform Error to Owner's Status handler */
              if (ssock_cb->ssock_status_func)
                ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);
              ret = 0;
              break;

            case EINTR:
            case EBADF:
            case EFAULT:
            case ENOTSOCK:
            case EAGAIN:
#if (EWOULDBLOCK != EAGAIN)
            case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */
            case EAFNOSUPPORT:
            case ECONNREFUSED:
            case ETIMEDOUT:
            case ENETUNREACH:
            case EADDRINUSE:
            default:
              if (ssock_cb->ssock_status_func)
                ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);
              stream_sock_cb_reset (ssock_cb, zlg);
              ssock_cb->ssock_state = SSOCK_STATE_IDLE;
              ret = 0;
              break;
            }
        }
      else if (ssock_cb->ssock_state == SSOCK_STATE_ACTIVE)
        {
          /* Obtian complete Socket Identification */
          ret = stream_sock_cb_get_id (ssock_cb, zlg);
          if (ret < 0)
            sock_errno = errno;

          /* Inform of Sock-CB owner of Socket Status */
          if (ssock_cb->ssock_status_func)
            ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);

          if (ret < 0)
            {
              ret = 0;
              goto EXIT;
            }

          CQUEUE_BUF_RESET (ssock_cb->ssock_ibuf);

          ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;

          /* Now start the 'read' thread */
          SSOCK_CB_READ_ON (zlg, ssock_cb->t_ssock_read, ssock_cb,
                            stream_sock_cb_read, ssock_cb->ssock_fd);
        }
      else /* ==> Sock-CB and Socket are already Connected */
        {
          sock_errno = EISCONN;

          /* Inform of Sock-CB owner of Socket Status */
          if (ssock_cb->ssock_status_func)
            ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);

          ret = 0;
        }
      break;

    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      ret = -1;
      break;
    }

EXIT:

  return ret;
}

int32_t
stream_sock_cb_accept (struct stream_sock_cb *ssock_cb,
                       sock_handle_t ssock_fd,
                       struct abc *zlg)
{
  int32_t sock_errno;
  int32_t ret;

  sock_errno = 0;
  ret = 0;

  /* Sanity check */
  if (! ssock_cb || ! zlg || ssock_fd < 0)
    {
      ret = -1;
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
    case SSOCK_STATE_ACTIVE:
      /* Reset Sock-Cb dynamic information */
      stream_sock_cb_reset (ssock_cb, zlg);

      /* Take on the new Socket FD */
      ssock_cb->ssock_fd = ssock_fd;

      /* Obtian complete Socket Identification */
      ret = stream_sock_cb_get_id (ssock_cb, zlg);
      if (ret < 0)
        sock_errno = errno;

      /* Inform of Sock-CB owner of Socket Status */
      if (ssock_cb->ssock_status_func)
        ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);

      if (ret < 0)
        {
          ret = 0;
          goto EXIT;
        }

      ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;

      /* Relaunch the Read Thread */
      SSOCK_CB_READ_ON (zlg, ssock_cb->t_ssock_read, ssock_cb,
                        stream_sock_cb_read, ssock_cb->ssock_fd);
      break;

    case SSOCK_STATE_CONNECTED:
    case SSOCK_STATE_WRITING:
    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      ret = -1;
      break;
    }

EXIT:

  return ret;
}

struct cqueue_buffer *
stream_sock_cb_get_write_cq_buf_prio (struct stream_sock_cb *ssock_cb,
                                 u_int32_t buf_size_req,
                                 struct abc *zlg, enum ssock_cb_priority priority)
{
  struct cqueue_buffer *cq_buf;
  int32_t ret;

  cq_buf = NULL;

  /* Sanity check */
  if (! zlg || ! ssock_cb)
    {
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      /* Return CQBuf of NULL */
      break;

    case SSOCK_STATE_CONNECTED:
    case SSOCK_STATE_WRITING:
      if (priority == SSOCK_PRIO_HIGH)
        cq_buf = CQUEUE_BUF_GET_LIST_TAIL_NODE (ssock_cb->ssock_obuf_high_list);
      else
        cq_buf = CQUEUE_BUF_GET_LIST_TAIL_NODE (ssock_cb->ssock_obuf_list);


      if (! cq_buf
          || buf_size_req > CQUEUE_BUF_GET_BYTES_EMPTY (cq_buf))
        {
          cq_buf = cqueue_buf_get (ssock_cb->ssock_buf_size, zlg);
          if (! cq_buf)
            goto EXIT;

          /* Enlist into OBuf List */
          if (priority == SSOCK_PRIO_HIGH)
            ret = cqueue_buf_listnode_add (ssock_cb->ssock_obuf_high_list,
                                           cq_buf, zlg);
          else
            ret = cqueue_buf_listnode_add (ssock_cb->ssock_obuf_list,
                                           cq_buf, zlg);
          if (ret < 0)
            {
              cqueue_buf_release (cq_buf, zlg);
              cq_buf = NULL;
              goto EXIT;
            }
        }
      break;
    }

EXIT:

  return cq_buf;
}

struct cqueue_buffer *
stream_sock_cb_get_write_cq_buf (struct stream_sock_cb *ssock_cb,
                                 u_int32_t buf_size_req,
                                 struct abc *zlg)
{
  return stream_sock_cb_get_write_cq_buf_prio (ssock_cb, buf_size_req, zlg, SSOCK_PRIO_MIDDLE);
}

/* Releases all unsent-whole CQueue buffers in write queue */
int32_t
stream_sock_cb_purge_unsent_bufs (struct stream_sock_cb *ssock_cb,
                                  struct abc *zlg)
{
  struct cqueue_buffer *cq_buf_nxt;
  struct cqueue_buffer *cq_buf;
  int32_t ret;

  cq_buf_nxt = NULL;
  cq_buf = NULL;
  ret = 0;

  /* Sanity check */
  if (! zlg || ! ssock_cb)
    {
      ret = -1;
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      ret = -1;
      break;

    case SSOCK_STATE_CONNECTED:
    case SSOCK_STATE_WRITING:
      cq_buf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_list);

      /* Retain First buffer since it might have partially sent Msgs */
      if (cq_buf)
        {
          cq_buf_nxt = cq_buf->next;
          cq_buf->next = NULL;

          for (cq_buf = cq_buf_nxt; cq_buf; cq_buf = cq_buf_nxt)
            {
              cq_buf_nxt = cq_buf->next;
              cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_list,
                                          cq_buf, zlg);
              cqueue_buf_release (cq_buf, zlg);
            }
        }

      cq_buf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_high_list);
      /* Retain First buffer since it might have partially sent Msgs */
      if (cq_buf)
        {
          cq_buf_nxt = cq_buf->next;
          cq_buf->next = NULL;

          for (cq_buf = cq_buf_nxt; cq_buf; cq_buf = cq_buf_nxt)
            {
              cq_buf_nxt = cq_buf->next;
              cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_high_list,
                                          cq_buf, zlg);
              cqueue_buf_release (cq_buf, zlg);
            }
        }

      break;
    }

EXIT:

  return ret;
}

int32_t
stream_sock_cb_write_mesg (struct stream_sock_cb *ssock_cb,
                           struct abc *zlg)
{
  int32_t ret;

  ret = 0;

  /* Sanity check */
  if (! zlg || ! ssock_cb)
    {
      ret = -1;
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      ret = -1;
      break;

    case SSOCK_STATE_CONNECTED:
      ssock_cb->ssock_state = SSOCK_STATE_WRITING;
      /* Do not break here */
    case SSOCK_STATE_WRITING:
      SSOCK_CB_WRITE_ON (zlg, ssock_cb->t_ssock_write, ssock_cb,
                         stream_sock_cb_write, ssock_cb->ssock_fd);
      break;
    }

EXIT:

  return ret;
}

#ifndef HAVE_BGP_THREAD_POOL
/* Instead of creating a write thread tries to write data onto socket
 * immediately. If failed then only add write thread
 */
int32_t
stream_sock_cb_write_mesg_immediate (struct stream_sock_cb *ssock_cb,
                                     struct abc *zlg)
{
  int32_t ret;

  ret = 0;

  /* Sanity check */
  if (! zlg || ! ssock_cb)
    {
      ret = -1;
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      ret = -1;
      break;

    case SSOCK_STATE_CONNECTED:
      ssock_cb->ssock_state = SSOCK_STATE_WRITING;
      /* Do not break here */
    case SSOCK_STATE_WRITING:
      SSOCK_CB_WRITE_IMMEDIATE (zlg, ssock_cb->t_ssock_write, ssock_cb,
                                stream_sock_cb_write, ssock_cb->ssock_fd);
      break;
    }

EXIT:

  return ret;
}
#endif /* !HAVE_BGP_THREAD_POOL */

int32_t
stream_sock_cb_close_repair (struct stream_sock_cb *ssock_cb,
                             struct abc *zlg)
{
  struct cqueue_buffer *cq_buf_nxt;
  struct cqueue_buffer *cq_buf;
  int32_t ret;
  ret = 0;

  if (! ssock_cb || ! zlg)
    {
      ret = -1;
      goto EXIT;
    }

  SSOCK_CB_READ_OFF (zlg, ssock_cb->t_ssock_read);
  SSOCK_CB_WRITE_OFF (zlg, ssock_cb->t_ssock_write);

  if (ssock_cb->ssock_fd >= 0)
    {
      close (ssock_cb->ssock_fd);
    }

  CQUEUE_BUF_RESET (ssock_cb->ssock_ibuf);

  /* Retain one CQ-Write Buf and release all the others */
  cq_buf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_list);
  if (cq_buf)
    {
      CQUEUE_BUF_RESET (cq_buf);

      cq_buf_nxt = cq_buf->next;
      cq_buf->next = NULL;

      for (cq_buf = cq_buf_nxt; cq_buf; cq_buf = cq_buf_nxt)
        {
          cq_buf_nxt = cq_buf->next;
          cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_list,
                                      cq_buf, zlg);
          cqueue_buf_release (cq_buf, zlg);
        }
    }

  cq_buf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_high_list);
  if (cq_buf)
    {
      CQUEUE_BUF_RESET (cq_buf);

      cq_buf_nxt = cq_buf->next;
      cq_buf->next = NULL;

      for (cq_buf = cq_buf_nxt; cq_buf; cq_buf = cq_buf_nxt)
        {
          cq_buf_nxt = cq_buf->next;
          cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_high_list,
                                      cq_buf, zlg);
          cqueue_buf_release (cq_buf, zlg);
        }
    }

  /* Reset Socket's dynamic information */
  ssock_cb->ssock_fd = -1;
  ssock_cb->ssock_read_func = NULL;
  ssock_cb->ssock_state = SSOCK_STATE_IDLE; 

EXIT:

  return ret;

}

int32_t
stream_sock_cb_close (struct stream_sock_cb *ssock_cb,
                      struct abc *zlg)
{
  int32_t ret;

  ret = 0;

  /* Sanity check */
  if (! zlg || ! ssock_cb)
    {
      ret = -1;
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
      /* Just return */
      break;

    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CONNECTED:
      stream_sock_cb_reset (ssock_cb, zlg);
      ssock_cb->ssock_state = SSOCK_STATE_IDLE;
      break;

    case SSOCK_STATE_WRITING:
      SSOCK_CB_READ_OFF (zlg, ssock_cb->t_ssock_read);
      ssock_cb->ssock_read_func = NULL;
      CQUEUE_BUF_RESET (ssock_cb->ssock_ibuf);
      memset (&ssock_cb->ssock_su_local, 0,
                   sizeof (union sockunion));
      memset (&ssock_cb->ssock_su_remote, 0,
                   sizeof (union sockunion));
      ssock_cb->ssock_state = SSOCK_STATE_CLOSING;
      break;

    case SSOCK_STATE_CLOSING:
    case SSOCK_STATE_ZOMBIE:
      ret = -1;
      break;
    }

EXIT:

  return ret;
}

void
stream_sock_cb_free (struct stream_sock_cb *ssock_cb,
                     struct abc *zlg)
{

  /* Sanity check */
  if (! ssock_cb || ! zlg)
    {
      goto EXIT;
    }

  switch (ssock_cb->ssock_state)
    {
    case SSOCK_STATE_IDLE:
    case SSOCK_STATE_ACTIVE:
    case SSOCK_STATE_CONNECTED:
      stream_sock_cb_delete (ssock_cb, zlg);
      break;

    case SSOCK_STATE_WRITING:
    case SSOCK_STATE_CLOSING:
      SSOCK_CB_READ_OFF (zlg, ssock_cb->t_ssock_read);
      ssock_cb->ssock_cb_owner = NULL;
      ssock_cb->ssock_read_func = NULL;
      ssock_cb->ssock_status_func = NULL;
      memset (&ssock_cb->ssock_su_local, 0,
                   sizeof (union sockunion));
      memset (&ssock_cb->ssock_su_remote, 0,
                   sizeof (union sockunion));
      listnode_add (SSOCK_CB_GET_ZOMBIE_LIST (zlg), ssock_cb);
      ssock_cb->ssock_state = SSOCK_STATE_ZOMBIE;
      break;

    case SSOCK_STATE_ZOMBIE:
      /* Do nothing */
      break;
    }

EXIT:

  return;
}

int32_t
stream_sock_cb_idle (struct stream_sock_cb *ssock_cb,
                     struct abc *zlg)
{
  int32_t ret;

  ret = 0;

  if (! ssock_cb || ! zlg)
    {
      ret = -1;
      goto EXIT;
    }

  stream_sock_cb_reset (ssock_cb, zlg);
  ssock_cb->ssock_state = SSOCK_STATE_IDLE;

EXIT:

  return ret;
}

/*
 * NOTE : NONE OF THE FUNCTIONS BELOW SHOULD TO BE INVOKED
 *        OUTSIDE THIS FILE (These are not Sock-CB APIs)
 */
int32_t
stream_sock_cb_get_id (struct stream_sock_cb *ssock_cb,
                       struct abc *zlg)
{
  socklen_t saddr_namelen;
  int32_t ret;
#ifdef HAVE_IPV6
  struct sockaddr_in6 *tmp_sin6;
  struct sockaddr_in tmp_sin;
  memset (&tmp_sin, 0, sizeof (struct sockaddr_in));
#endif /* HAVE_IPV6 */

  memset (&ssock_cb->ssock_su_local, 0, sizeof (union sockunion));
  saddr_namelen = sizeof (union sockunion);

  /* Obtain Local-Address/Name for the connected Socket */
  ret = getsockname (ssock_cb->ssock_fd,
                   (struct sockaddr *)&ssock_cb->ssock_su_local,
                   &saddr_namelen);
  if (ret < 0)
    {
      goto EXIT;
    }

#ifdef HAVE_IPV6
  if (ssock_cb->ssock_su_local.sa.sa_family == AF_INET6)
    {
      tmp_sin6 = &ssock_cb->ssock_su_local.sin6;
      if (IN6_IS_ADDR_V4MAPPED (&tmp_sin6->sin6_addr))
        {
          tmp_sin.sin_family = AF_INET;
          memcpy (&tmp_sin.sin_addr,
                       &((u_int32_t *) &tmp_sin6->sin6_addr) [3],
                       IPV4_MAX_BYTELEN);
          tmp_sin.sin_port = tmp_sin6->sin6_port;
          memcpy (&ssock_cb->ssock_su_local.sin, &tmp_sin,
                       sizeof (struct sockaddr_in));
        }
    }
#endif /* HAVE_IPV6 */

  memset (&ssock_cb->ssock_su_remote, 0, sizeof (union sockunion));
  saddr_namelen = sizeof (union sockunion);

  /* Obtain Remote-Address/Name for the connected Socket */
  ret = getpeername (ssock_cb->ssock_fd,
                  (struct sockaddr *)&ssock_cb->ssock_su_remote,
                  &saddr_namelen);
  
  if (ret < 0)
    {
      goto EXIT;
    }

#ifdef HAVE_IPV6
  if (ssock_cb->ssock_su_remote.sa.sa_family == AF_INET6)
    {
      tmp_sin6 = &ssock_cb->ssock_su_remote.sin6;
      if (IN6_IS_ADDR_V4MAPPED (&tmp_sin6->sin6_addr))
        {
          tmp_sin.sin_family = AF_INET;
          memcpy (&tmp_sin.sin_addr,
                       &((u_int32_t *) &tmp_sin6->sin6_addr) [3],
                       IPV4_MAX_BYTELEN);
          tmp_sin.sin_port = tmp_sin6->sin6_port;
          memcpy (&ssock_cb->ssock_su_remote.sin, &tmp_sin,
                       sizeof (struct sockaddr_in));
        }
    }
#endif /* HAVE_IPV6 */

EXIT:

  return ret;
}


/* Resets Socket-CB dynamic contents */
int32_t
stream_sock_cb_reset (struct stream_sock_cb *ssock_cb,
                      struct abc *zlg)
{
  struct cqueue_buffer *cq_buf_nxt;
  struct cqueue_buffer *cq_buf;
  int32_t sock_read;
  int32_t ret;

  ret = 0;

  if (! ssock_cb || ! zlg)
    {
      ret = -1;
      goto EXIT;
    }

  SSOCK_CB_READ_OFF (zlg, ssock_cb->t_ssock_read);
  SSOCK_CB_WRITE_OFF (zlg, ssock_cb->t_ssock_write);

  /* Read-out all incoming Socket Buffer and retain 'ssock_ibuf' */
  if (ssock_cb->ssock_fd >= 0)
    {
      set_nonblocking (ssock_cb->ssock_fd);
      do {
        CQUEUE_BUF_RESET (ssock_cb->ssock_ibuf);
        sock_read = read (ssock_cb->ssock_fd,
                             ssock_cb->ssock_ibuf->data,
                             ssock_cb->ssock_ibuf->size);
      } while (sock_read > 0);

      shutdown (ssock_cb->ssock_fd, SHUT_RD);
      shutdown (ssock_cb->ssock_fd, SHUT_WR);
      close (ssock_cb->ssock_fd);
    }
  CQUEUE_BUF_RESET (ssock_cb->ssock_ibuf);

  /* Retain one CQ-Write Buf and release all the others */
  cq_buf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_list);
  if (cq_buf)
    {
      CQUEUE_BUF_RESET (cq_buf);

      cq_buf_nxt = cq_buf->next;
      cq_buf->next = NULL;

      for (cq_buf = cq_buf_nxt; cq_buf; cq_buf = cq_buf_nxt)
        {
          cq_buf_nxt = cq_buf->next;
          cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_list,
                                      cq_buf, zlg);
          cqueue_buf_release (cq_buf, zlg);
        }
    }

  cq_buf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_high_list);
  if (cq_buf)
    {
      CQUEUE_BUF_RESET (cq_buf);

      cq_buf_nxt = cq_buf->next;
      cq_buf->next = NULL;

      for (cq_buf = cq_buf_nxt; cq_buf; cq_buf = cq_buf_nxt)
        {
          cq_buf_nxt = cq_buf->next;
          cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_high_list,
                                      cq_buf, zlg);
          cqueue_buf_release (cq_buf, zlg);
        }
    }

  /* Reset Socket's dynamic information */
  ssock_cb->ssock_fd = -1;
  ssock_cb->ssock_read_func = NULL;
  memset (&ssock_cb->ssock_su_local, 0, sizeof (union sockunion));
  memset (&ssock_cb->ssock_su_remote, 0, sizeof (union sockunion));

EXIT:

  return ret;
}

void
stream_sock_cb_delete (struct stream_sock_cb *ssock_cb,
                       struct abc *zlg)
{
  int32_t sock_read;

  if (! ssock_cb || ! zlg)
    goto EXIT;

  SSOCK_CB_READ_OFF (zlg, ssock_cb->t_ssock_read);
  SSOCK_CB_WRITE_OFF (zlg, ssock_cb->t_ssock_write);

  if (ssock_cb->ssock_fd >= 0)
    {
      set_nonblocking (ssock_cb->ssock_fd);
      do {
        CQUEUE_BUF_RESET (ssock_cb->ssock_ibuf);
        sock_read = read (ssock_cb->ssock_fd,
                                   ssock_cb->ssock_ibuf->data,
                                   ssock_cb->ssock_ibuf->size);
      } while (sock_read > 0);

      shutdown (ssock_cb->ssock_fd, SHUT_RD);
      shutdown (ssock_cb->ssock_fd, SHUT_WR);
      close (ssock_cb->ssock_fd);
    }

  cqueue_buf_release (ssock_cb->ssock_ibuf, zlg);

  if (ssock_cb->ssock_obuf_list)
    cqueue_buf_list_free (ssock_cb->ssock_obuf_list, zlg);

  if (ssock_cb->ssock_obuf_high_list)
    cqueue_buf_list_free (ssock_cb->ssock_obuf_high_list, zlg);

  XFREE (MTYPE_SSOCK_CB,  ssock_cb);

EXIT:

  return;
}

int32_t
stream_sock_cb_read (struct thread *t_ssock_read)
{
  struct stream_sock_cb *ssock_cb;
  int32_t cqb_bytes_empty;
  struct abc *zlg;
  int32_t sock_readsize;
  int32_t sock_errno;
  enum ssock_error ret;
  int32_t sock_read;
  int32_t ret_val;

  sock_errno = 0;
  ret = SSOCK_ERR_NONE;
  ssock_cb = NULL;
  sock_read = 0;
  ret_val = 0;
  zlg = NULL;

  /* Obtain the SOCK Lib Global */
  zlg = THREAD_GLOB (t_ssock_read);

  /* Sanity check */
  if (! zlg)
    {
      ret = SSOCK_ERR_INVALID;
      goto EXIT;
    }

  /* Obtain the SOCK-CB */
  ssock_cb = THREAD_ARG (t_ssock_read);

  /* Sanity check */
  if (! ssock_cb || 0 > ssock_cb->ssock_fd )
    {
      ret = -1;
      goto EXIT;
    }

  /* Reset the Thread */
  ssock_cb->t_ssock_read = NULL;

READ_AGAIN:

  /* Get bytes empty in CQBuf */
  cqb_bytes_empty = CQUEUE_BUF_GET_BYTES_EMPTY (ssock_cb->ssock_ibuf);

  /* Read max possible size */
  sock_readsize = CQUEUE_BUF_GET_CONTIG_BYTES_EMPTY (ssock_cb->ssock_ibuf);

  /* Socket 'read' System Call */
  if (sock_readsize)
    sock_read = read (ssock_cb->ssock_fd,
                               (ssock_cb->ssock_ibuf->data +
                                ssock_cb->ssock_ibuf->putp),
                               sock_readsize);

  /* Process the return value */
  if (sock_read < 0)
    {
      sock_errno = errno;

      switch (sock_errno)
        {
        case 0:
        case EAGAIN:
#if (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */
        case EINTR:
            ret = SSOCK_ERR_NONE;
            break;

        case EFAULT:
        case EINVAL:
        case EBADF:
        case EISDIR:
        case EIO:
        case ENOTCONN:
        case -1:
        default:
            if (ssock_cb->ssock_status_func)
              ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);
            stream_sock_cb_reset (ssock_cb, zlg);
            ssock_cb->ssock_state = SSOCK_STATE_IDLE;
            ret = SSOCK_ERR_CLOSE;
            break;
        }
    }
  else if (sock_read > 0)
    {
      /* Advance the Input CQ-Buffer position */
      CQUEUE_WRITE_ADVANCE_NBYTES (ssock_cb->ssock_ibuf, sock_read);

      /* If SOCK not already connected, first inform the owner */
      if (ssock_cb->ssock_state == SSOCK_STATE_ACTIVE)
        {
          /* Obtian complete Socket Identification */
          ret_val = stream_sock_cb_get_id (ssock_cb, zlg);
          if (ret_val < 0)
            sock_errno = errno;

          if (ssock_cb->ssock_status_func)
            ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);

          if (ret_val < 0)
            {
              stream_sock_cb_reset (ssock_cb, zlg);
              ssock_cb->ssock_state = SSOCK_STATE_IDLE;
              ret = SSOCK_ERR_CLOSE;
              goto EXIT;
            }

          ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;
        }

      /* Invoke READ FUNC of the CB-owner */
      do {
        if (ssock_cb->ssock_read_func)
          ret = ssock_cb->ssock_read_func (ssock_cb,
                                           ssock_cb->ssock_read_func_arg,
                                           zlg);
      } while (ret == SSOCK_ERR_READ_LOOP
               && ssock_cb->ssock_read_func);

      /* Should read ALL readable data from the socket */
      if (sock_read == sock_readsize && sock_readsize < cqb_bytes_empty)
        goto READ_AGAIN;
    }
  else /* ==> sock_read == 0 */
    {
      if (ssock_cb->ssock_state == SSOCK_STATE_ACTIVE)
        {
          /* Obtian complete Socket Identification */
          ret_val = stream_sock_cb_get_id (ssock_cb, zlg);
          if (ret_val < 0)
            sock_errno = errno;

          if (ssock_cb->ssock_status_func)
            ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);

          if (ret_val < 0)
            {
              stream_sock_cb_reset (ssock_cb, zlg);
              ssock_cb->ssock_state = SSOCK_STATE_IDLE;
              ret = SSOCK_ERR_CLOSE;
              goto EXIT;
            }

          ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;
        }
      else if (sock_readsize)
        { /* ==> End-of-file */
          if (ssock_cb->ssock_status_func)
            ssock_cb->ssock_status_func (ssock_cb, ENOTCONN, zlg);
          stream_sock_cb_reset (ssock_cb, zlg);
          ssock_cb->ssock_state = SSOCK_STATE_IDLE;
          ret = SSOCK_ERR_CLOSE;
          goto EXIT;
        }
    }

EXIT:

  if (ret == SSOCK_ERR_NONE)
    SSOCK_CB_READ_ON (zlg, ssock_cb->t_ssock_read, ssock_cb,
                      stream_sock_cb_read, ssock_cb->ssock_fd);

  return 0;
}

int32_t
stream_sock_cb_write (struct thread *t_ssock_write)
{
  struct stream_sock_cb *ssock_cb;
  struct cqueue_buffer *cq_wbuf;
  int32_t sock_writesize;
  struct abc *zlg;
  int32_t sock_written;
  int32_t sock_errno;
  enum ssock_error ret;
  int32_t ret_val;
  int is_high = 0;

  sock_errno = 0;
  ret = SSOCK_ERR_NONE;
  sock_written = 0;
  ssock_cb = NULL;
  ret_val = 0;
  zlg = NULL;

  /* Obtain the SOCK Lib Global */
  zlg = THREAD_GLOB (t_ssock_write);

  /* Sanity check */
  if (! zlg)
    {
      ret = SSOCK_ERR_INVALID;
      goto EXIT;
    }

  /* Obtain the SOCK-CB */
  ssock_cb = THREAD_ARG (t_ssock_write);

  /* Sanity check */
  if (! ssock_cb || 0 > ssock_cb->ssock_fd )
    {
      ret = -1;
      goto EXIT;
    }

  /* Reset the Thread */
  ssock_cb->t_ssock_write = NULL;

WRITE_AGAIN:

  /* Get first CQ-Buf from obuf-list */
  cq_wbuf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_high_list);
  if (CQUEUE_BUF_GET_BYTES_TBR(cq_wbuf) == 0)
     {
      cq_wbuf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_list);
      if (! cq_wbuf)
        {
          ret = -1;
          goto EXIT;
        }
      is_high = 0;
    }
  else
    {
      is_high = 1;
    }

  /* Get max possible contiguous size of bytes to be written */
  sock_writesize = CQUEUE_BUF_GET_CONTIG_BYTES_TBR (cq_wbuf);

  /* Socket 'write' System Call */
  if (sock_writesize)
    sock_written = write (ssock_cb->ssock_fd,
                                   (cq_wbuf->data + cq_wbuf->getp),
                                   sock_writesize);

  /* Process the return value */
  if (sock_written < 0)
    {
      sock_errno = errno;

      switch (sock_errno)
        {
        case 0:
        case EAGAIN:
#if (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */
        case EINTR:
            ret = SSOCK_ERR_NONE;
            break;

        case EFAULT:
        case EINVAL:
        case EBADF:
        case EISDIR:
        case EIO:
        case ENOTCONN:
        default:
            if (ssock_cb->ssock_status_func)
              ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);
            stream_sock_cb_reset (ssock_cb, zlg);
            ssock_cb->ssock_state = SSOCK_STATE_IDLE;
            ret = SSOCK_ERR_CLOSE;
            break;
        }
    }
  else if (sock_written > 0)
    {
      /* Advance the Output CQ-Buffer position */
      CQUEUE_READ_ADVANCE_NBYTES (cq_wbuf, sock_written);

      /* If SOCK not already connected, first inform the owner */
      if (ssock_cb->ssock_state == SSOCK_STATE_ACTIVE)
        {
          /* Obtian complete Socket Identification */
          ret_val = stream_sock_cb_get_id (ssock_cb, zlg);
          if (ret_val < 0)
            sock_errno = errno;

          if (ssock_cb->ssock_status_func)
            ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);

          if (ret_val < 0)
            {
              stream_sock_cb_reset (ssock_cb, zlg);
              ssock_cb->ssock_state = SSOCK_STATE_IDLE;
              ret = SSOCK_ERR_CLOSE;
              goto EXIT;
            }

          ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;

          /* Now start the 'read' thread */
          SSOCK_CB_READ_ON (zlg, ssock_cb->t_ssock_read, ssock_cb,
                            stream_sock_cb_read, ssock_cb->ssock_fd);
        }

      if (! CQUEUE_BUF_GET_BYTES_TBR (cq_wbuf))
        {
          /*
           * When all bytes in current buffer are sent, free it
           * if additional buffers are in queue. Retain just one
           * buffer in the OBuf list
           */
          if (cq_wbuf->next)
            {
              if (is_high)
                cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_high_list,
                                            cq_wbuf, zlg);
              else
                cqueue_buf_listnode_remove (ssock_cb->ssock_obuf_list,
                                            cq_wbuf, zlg);

              cqueue_buf_release (cq_wbuf, zlg);

              /* If Socket consumed the entire chunk 'write' more */
              if (is_high && (sock_written == sock_writesize))
                goto WRITE_AGAIN;
            }
          else if (is_high == 0 ||
                   ((cq_wbuf = CQUEUE_BUF_GET_LIST_HEAD_NODE (ssock_cb->ssock_obuf_list)) == NULL) ||
                   ((CQUEUE_BUF_GET_BYTES_TBR(cq_wbuf) == 0) &&
                   (cq_wbuf->next == NULL)))
            { /* => No more data to send */
              switch (ssock_cb->ssock_state)
                {
                case SSOCK_STATE_IDLE:
                case SSOCK_STATE_ACTIVE:
                case SSOCK_STATE_CONNECTED:
                  /* Do no restart 'write' thread */
                  ret = SSOCK_ERR_CLOSE;
                  break;

                case SSOCK_STATE_WRITING:
                  ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;

                  /* Do no restart 'write' thread */
                  ret = SSOCK_ERR_CLOSE;
                  break;

                case SSOCK_STATE_CLOSING:
                  stream_sock_cb_reset (ssock_cb, zlg);
                  ssock_cb->ssock_state = SSOCK_STATE_IDLE;

                  /* Do no restart 'write' thread */
                  ret = SSOCK_ERR_CLOSE;
                  break;

                case SSOCK_STATE_ZOMBIE:
                  /*
                   * We need to restart 'write' thread one last time
                   * in-order to give time for Socket to send out
                   * whatever we just finished writing
                   */
                  ret = SSOCK_ERR_NONE;
                  break;
                }
            }
        }
      /* If Socket consumed the entire chunk 'write' more */
      else if (sock_written == sock_writesize)
        goto WRITE_AGAIN;
    }
  else /* ==> sock_written == 0 */
    {
      if (ssock_cb->ssock_state == SSOCK_STATE_ACTIVE)
        {
          /* Obtian complete Socket Identification */
          ret_val = stream_sock_cb_get_id (ssock_cb, zlg);
          if (ret_val < 0)
            sock_errno = errno;

          if (ssock_cb->ssock_status_func)
            ssock_cb->ssock_status_func (ssock_cb, sock_errno, zlg);

          if (ret_val < 0)
            {
              ret = SSOCK_ERR_CLOSE;
              goto EXIT;
            }

          ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;

          /* Now start the 'read' thread */
          SSOCK_CB_READ_ON (zlg, ssock_cb->t_ssock_read, ssock_cb,
                            stream_sock_cb_read, ssock_cb->ssock_fd);

          /* Dont restart 'write' thread unless we have data to send */
          ret = SSOCK_ERR_CLOSE;
        }

      if (! sock_writesize)
        { /* => No more data to send */
          switch (ssock_cb->ssock_state)
            {
            case SSOCK_STATE_IDLE:
            case SSOCK_STATE_ACTIVE:
            case SSOCK_STATE_CONNECTED:
              break;

            case SSOCK_STATE_WRITING:
              ssock_cb->ssock_state = SSOCK_STATE_CONNECTED;
              break;

            case SSOCK_STATE_CLOSING:
              stream_sock_cb_reset (ssock_cb, zlg);
              ssock_cb->ssock_state = SSOCK_STATE_IDLE;
              break;

            case SSOCK_STATE_ZOMBIE:
              listnode_delete (SSOCK_CB_GET_ZOMBIE_LIST (zlg),
                               ssock_cb);
              stream_sock_cb_delete (ssock_cb, zlg);
              break;
            }

          /* Do no restart 'write' thread */
          ret = SSOCK_ERR_CLOSE;
        }
    }

EXIT:

  if (ret == SSOCK_ERR_INVALID || ssock_cb == NULL)
  {
        /* Do nothing here */
  }
  else if (ret == SSOCK_ERR_NONE)
    SSOCK_CB_WRITE_ON (zlg, ssock_cb->t_ssock_write, ssock_cb,
                       stream_sock_cb_write, ssock_cb->ssock_fd);
  else if (ssock_cb->ssock_tx_ready_report && ssock_cb->ssock_tx_ready_func)
        ssock_cb->ssock_tx_ready_func (ssock_cb, zlg);

  return 0;
}
