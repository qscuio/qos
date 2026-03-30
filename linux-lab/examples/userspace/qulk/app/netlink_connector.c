#include <sys/types.h>
#include <sys/socket.h>
#include <linux/cn_proc.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/filter.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/uio.h>

#define offsetof(a,b) ((unsigned long)(&(((a *)0)->b)))
#define PAGE_SIZE 4096
enum proc_cn_mcast_op op;
int
sock_connector (void)
{
  int sock;
  struct iovec iov[3];
  char nlmsghdrbuf[NLMSG_LENGTH (0)];
  struct nlmsghdr *nlmsghdr = (void *) nlmsghdrbuf;
  struct cn_msg cn_msg;
  struct sockaddr_nl addr;
  sock = socket (PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_CONNECTOR);
  addr.nl_family = AF_NETLINK;
  addr.nl_pid = getpid ();
  addr.nl_groups = CN_IDX_PROC;
  bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  nlmsghdr->nlmsg_len = NLMSG_LENGTH (sizeof(cn_msg)+ sizeof(op));
  nlmsghdr->nlmsg_type = NLMSG_DONE;
  nlmsghdr->nlmsg_flags = 0;
  nlmsghdr->nlmsg_seq = 0;
  nlmsghdr->nlmsg_pid = getpid ();
  iov[0].iov_base = nlmsghdrbuf;
  iov[0].iov_len = NLMSG_LENGTH (0);
  cn_msg.id.idx = CN_IDX_PROC;
  cn_msg.id.val = CN_VAL_PROC;
  cn_msg.seq = 0;
  cn_msg.ack = 0;
  cn_msg.len = sizeof(op);
  iov[1].iov_base = &cn_msg;
  iov[1].iov_len = sizeof(cn_msg);
  op = PROC_CN_MCAST_LISTEN;
  iov[2].iov_base = &op;
  iov[2].iov_len = sizeof(op);
  writev (sock, iov, 3);
  return sock;
}
#define BPF_INSTR_SIZE sizeof(struct sock_filter)
int
set_packet_filter (int sock, pid_t pid_list[], int npid)
{
  struct sock_filter bpf_preamble[] = {
    /* if nlmsg_type != NLMSG_DOONE, then return 0(discard) */
    BPF_STMT (BPF_LD | BPF_H | BPF_ABS,
              offsetof (struct nlmsghdr, nlmsg_type)),
    BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K, htons (NLMSG_DONE), 1, 0),
    BPF_STMT (BPF_RET | BPF_K, 0),
    /* if cn_msg.id != CN_IDX_PROC, then return 0(discard) */
    BPF_STMT (BPF_LD | BPF_W | BPF_ABS,
              NLMSG_LENGTH (0) + offsetof (struct cn_msg, id)
              + offsetof (struct cb_id, idx)),
    BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K, htonl (CN_IDX_PROC), 1, 0),
    BPF_STMT (BPF_RET | BPF_K, 0),
    /* if proc_event.what != PROC_EVENT_EXIT, then return 0(discard) */
    BPF_STMT (BPF_LD | BPF_W | BPF_ABS,
              NLMSG_LENGTH (0) + offsetof (struct cn_msg, data)
              + offsetof (struct proc_event, what)),
    BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K, htonl (PROC_EVENT_EXIT), 1, 0),
    BPF_STMT (BPF_RET | BPF_K, 0),
    /* load process_pid value */
    BPF_STMT (BPF_LD | BPF_W | BPF_ABS,
              NLMSG_LENGTH (0) + offsetof (struct cn_msg, data)
              + offsetof (struct proc_event, event_data)
              + offsetof (struct exit_proc_event, process_pid)),
  };
  /* if exit_pid == pid, then return maximum else continue for next pid */
  struct sock_filter bpf_pid_template[] = {
    BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K, 0, 0, 1),
    BPF_STMT (BPF_RET | BPF_K, ~0)
  };
  /* if no match, then return 0 */
  const static struct sock_filter bpf_postamble[] = {
    BPF_STMT (BPF_RET | BPF_K, 0)
  };
  struct sock_fprog fprog;
  struct sock_filter *bpf;
  int i, bpf_size, pc, ret;
  bpf_size =
    sizeof(bpf_pid_template) * npid + sizeof(bpf_preamble) +
    sizeof(bpf_postamble);
  bpf = malloc (bpf_size);
  pc = 0;
  memcpy (bpf, bpf_preamble, sizeof(bpf_preamble));
  pc += sizeof(bpf_preamble) / BPF_INSTR_SIZE;
  for (i = 0; i < npid;
       i++, pc += sizeof(bpf_pid_template) / BPF_INSTR_SIZE)
    {
      bpf_pid_template[0].k = htonl (pid_list[i]);
      memcpy (bpf + pc, bpf_pid_template,
              sizeof(bpf_pid_template));
    }
  memcpy (bpf + pc, bpf_postamble, sizeof(bpf_postamble));
  fprog.filter = bpf;
  fprog.len = bpf_size / sizeof(struct sock_filter);
  ret = setsockopt (sock, SOL_SOCKET, SO_ATTACH_FILTER, &fprog,
                     sizeof(fprog));
  free(bpf);
  return ret;
}
void
recv_loop (int sock)
{
  struct nlmsghdr *nlmsghdr;
  struct msghdr msghdr;
  struct sockaddr_nl addr;
  struct iovec iov[1];
  char buf[PAGE_SIZE];
  ssize_t len;
  for (;;)
    {
      msghdr.msg_name = &addr;
      msghdr.msg_namelen = sizeof(addr);
      msghdr.msg_iov = iov;
      msghdr.msg_iovlen = 1;
      msghdr.msg_control = NULL;
      msghdr.msg_controllen = 0;
      msghdr.msg_flags = 0;
      iov[0].iov_base = buf;
      iov[0].iov_len = sizeof(buf);
      len = recvmsg (sock, &msghdr, 0);
      if (addr.nl_pid != 0)
        continue;
      for (nlmsghdr = (struct nlmsghdr *) buf; NLMSG_OK (nlmsghdr, len);
           nlmsghdr = NLMSG_NEXT (nlmsghdr, len))
        {
          struct cn_msg *cn_msg_p = NLMSG_DATA (nlmsghdr);
          struct proc_event *ev = (struct proc_event *) cn_msg_p->data;
          if ((nlmsghdr->nlmsg_type == NLMSG_ERROR)
              || (nlmsghdr->nlmsg_type == NLMSG_NOOP))
            continue;
          if ((cn_msg_p->id.idx != CN_IDX_PROC)
              || (cn_msg_p->id.val != CN_VAL_PROC))
            continue;
          switch (ev->what)
            {
            case PROC_EVENT_FORK:
              printf ("FORK %d/%d -> %d/%d\n",
                      ev->event_data.fork.parent_pid,
                      ev->event_data.fork.parent_tgid,
                      ev->event_data.fork.child_pid,
                      ev->event_data.fork.child_tgid);
              break;
            case PROC_EVENT_EXEC:
              printf ("EXEC %d/%d\n",
                      ev->event_data.exec.process_pid,
                      ev->event_data.exec.process_tgid);
              break;
            case PROC_EVENT_EXIT:
              printf ("EXIT %d/%d with exit code/signal %d/%d\n",
                      ev->event_data.exit.process_pid,
                      ev->event_data.exit.process_tgid,
                      ev->event_data.exit.exit_code,
                      ev->event_data.exit.exit_signal);
              break;
            case PROC_EVENT_COREDUMP:
              printf ("COREDUMP %d/%d\n",
                      ev->event_data.coredump.process_pid,
                      ev->event_data.coredump.process_tgid);
              break;
            case PROC_EVENT_PTRACE:
              printf ("PTRACE %d/%d by %d/%d\n",
                      ev->event_data.ptrace.process_pid,
                      ev->event_data.ptrace.process_tgid,
                      ev->event_data.ptrace.tracer_pid,
                      ev->event_data.ptrace.tracer_tgid);
              break;
            case PROC_EVENT_UID:
            case PROC_EVENT_NONE:
            case PROC_EVENT_GID:
            case PROC_EVENT_SID:
            case PROC_EVENT_COMM:
            default:
              break;
            }
        }
    }
}
int
main (int argc, char *argv[])
{
  int sd;
  pid_t pid_list[100];
  int i;
  for (i = 1; i < argc; i++)
    pid_list[i - 1] = strtoul (argv[i], NULL, 0);
  sd = sock_connector ();
  set_packet_filter (sd, pid_list, argc - 1);
  recv_loop (sd);
  exit (0);
}
