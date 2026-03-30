#ifndef __HAVE_CPM_CLIENT_H__
#define __HAVE_CPM_CLIENT_H__

/*
    +-------- nsm_server --------+
    | Message Handler            |
    | NSM services bit           |
    | NSM message parser         |
    | NSM message callback       |
    | vector of NSM server Client|
    +--------------|-------------+
                   |
                   v
    +------- nsm_server_client ----+
    |  NSM client ID               |-+
    |  nsm_server_entry for ASYNC  | |-+
    |  nsm_server_entry for SYNC   | | |
    +------------------------------+ | |
      +------------------------------+ |
        +------------------------------+
*/

#define CPM_PROTOCOL_VERSION_1               1
#define CPM_PROTOCOL_VERSION_2               2
#define CPM_MESSAGE_MAX_LEN               4096

#define CPM_CHECK_CTYPE(F,C)        (CHECK_FLAG (F, (1 << C)))
#define CPM_SET_CTYPE(F,C)          (SET_FLAG (F, (1 << C)))
#define CPM_UNSET_CTYPE(F,C)        (UNSET_FLAG (F, (1 << C)))

#define CPM_CHECK_CTYPE_FLAG(F,C)        (CHECK_FLAG (F, C))
#define CPM_SET_CTYPE_FLAG(F,C)          (SET_FLAG (F, C))
#define CPM_UNSET_CTYPE_FLAG(F,C)        (UNSET_FLAG (F, C))



struct message_entry;
struct cpm_client_handler;

/*
 * CPM (Common Protocol Messaging) header
 */
struct cpm_msg_header
{
  /* Message Type. */
  u_int16_t type;

  /* Message Len. */
  u_int16_t length;

  /* Message ID. */
  u_int32_t message_id;
};
#define CPM_MSG_HEADER_SIZE   ((int) sizeof(struct cpm_msg_header))

/*
 * Structure to hold pending message.
 */
struct cpm_client_pend_msg
{
  /* CPM Msg Header. */
  struct cpm_msg_header header;

  /* Message. */
  u_char buf[CPM_MESSAGE_MAX_LEN];
};

/*
 * CPM message send queue. - Move this to lib/cpm_message.h
 */
struct cpm_message_queue
{
  struct cpm_message_queue *next;
  struct cpm_message_queue *prev;

  u_char *buf;
  u_int16_t length;
  u_int16_t written;
};

/*
 * CPM services structure.
 */
struct cpm_msg_service
{
  /* TLV flags. */
  cindex_t cindex;

  /* NSM Protocol Version. */
  u_int16_t version;

  /* owner ID (e.g., RIBd) */
  u_int16_t owner_id;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* Client Id. */
  u_int32_t client_id;

  /* Service Bits. */
  u_int32_t bits;

#if 0
@@@
@@@ Owner specific information.
@@@
  /* Graceful Restart State */
  u_char restart_state;

  /* Graceful Restart TLV. */
  u_char restart[AFI_MAX][SAFI_MAX];

  /* Grace Perioud Expires TLV. */
  pal_time_t grace_period;

  /* Restart Option TLV.  */
  u_char *restart_val;
  u_int16_t restart_length;

#if (defined HAVE_MPLS || defined HAVE_GMPLS)
  /* Label pools used before restart. */
  struct nsm_msg_label_pool *label_pools;
  u_int16_t label_pool_num;
#endif /* HAVE_MPLS || HAVE_GMPLS */
#endif/*0*/
};

/* To Do :: Needs to be changed as application will maintain these details */
#define CPM_MSG_SERVICE_SIZE (16)
#define CPM_SERVICE_MAX (sizeof(((struct cpm_msg_service *)(0))->bits) << 3)

#define CPM_MESSAGE_MAX_LEN 4096



/*
 * CPM owner IDs (new)
 */
enum cpm_owner_id_type {
  CPM_OWNER_INVALID = 0,
  CPM_OWNER_RIB = 1,
  CPM_OWNER_OSPF = 2,
  CPM_OWNER_BGP = 3,
  CPM_OWNER_PIM = 4,
  CPM_OWNER_OSPF6 = 5,
  CPM_OWNER_RIP = 6,
  CPM_OWNER_RIPNG = 7,
  CPM_OWNER_ISIS = 8,
  CPM_OWNER_LDP = 9,
  CPM_OWNER_RSVP = 10,
  CPM_OWNER_SPB = 11,
  CPM_OWNER_NSM = 12,
  CPM_OWNER_VRRP = 13,
  CPM_OWNER_LAG = 14,
  CPM_OWNER_OAM = 15,
  CPM_OWNER_DHCP6 = 16,
#ifdef HAVE_OAM_SRV6
  CPM_OWNER_SRV6ONM = 17,
#endif /* HAVE_OAM_SRV6 */
#ifdef HAVE_SRV6
  CPM_OWNER_SRV6 = 18,
#endif
  CPM_OWNER_SRV6ONMRX = 19,

  CPM_OWNER_MAX
};


/*
 * CPM callback function typedef.
 */
typedef int (*CPM_DISCONNECT_CALLBACK) ();
typedef int (*CPM_CALLBACK) (struct cpm_msg_header *, void *, void *);
typedef int (*CPM_PARSER) (u_char **, u_int16_t *, struct cpm_msg_header *,
                           void *, CPM_CALLBACK);
typedef void (*cpm_client_handler_cb) (struct cpm_client_handler *);
typedef int (*CPM_HA_CALLBACK) (struct abc *zg, u_int16_t type);


/*
 * CPM client to server connection structure.
 * Each owner of the CPM client service must have its own handler
 * like as follows:
 *  struct rib_client_handler
 *  {
 *    struct cpm_client_handler cch;
 *
 *    RIBd specific members 
 *    ...
 *  };
 */
struct cpm_client_handler
{
  /* Parent. */
  struct cpm_client *cc;

  /* Up or down.  */
  int up;

  /* Type of client, sync or async.  */
  int type;

  /* Message client structure. */
  struct message_handler *mc;

  /* Service bits specific for this connection.  */
  struct cpm_msg_service service;

  /* Message buffer for output. */
  u_char buf[CPM_MESSAGE_MAX_LEN];
  u_int16_t len;
  u_int16_t size;
  u_char *pnt;

  /* Message buffer for input. */
  u_char buf_in[CPM_MESSAGE_MAX_LEN];
  u_int16_t len_in;
  u_int16_t size_in;
  u_char *pnt_in;
  struct cpm_msg_header header;

  /* Client message ID.  */
  u_int32_t message_id;

  /* List of pending messages of struct nsm_client_pend_msg. */
  struct list pend_msg_list;

  /* Send and recieved message count.  */
  u_int32_t send_msg_count;
  u_int32_t recv_msg_count;

  struct thread *pend_read_thread;
  bool pend_read_proc;
#if 0
@@@
@@@ Owner specific information
@@@

  /* Message ID for ILM/FTN entries. */
  u_int32_t mpls_msg_id;

  /* Message buffer for IPv4 route updates.  */
  u_char buf_ipv4[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv4;
  u_int16_t len_ipv4;
  struct thread *t_ipv4;

#ifdef HAVE_IPV6
  /* Message buffer for IPv6 route updates.  */
  u_char buf_ipv6[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv6;
  u_int16_t len_ipv6;
  struct thread *t_ipv6;
#endif /* HAVE_IPV6 */
#endif/*0*/
};

/*
 * CPM client structure.
 */
struct cpm_client
{
  struct abc *zg;

  /* CPM client ID. */
  u_int32_t client_id;

  /*
   * Async connection.
   * Pointing to struct cpm_client_hander in each
   * owner's client handler (e.g., struct rib_client_handler)
   * See cpm_client_set_service().
   */
   struct cpm_client_handler *async;
 
#ifdef HAVE_TCP_MESSAGE
  /* TCP port number  */
  u_int16_t port;
#else
  char *path;
#endif/* HAVE_TCP_MESSAGE */
  /* Owner ID */
  u_int16_t owner_id;

  /* Owner's client size in bytes (e.g., struct rib_client) */
  u_int16_t c_size;

  /* Owner's client handler size in bytes (e.g., struct rib_client_handler) */
  u_int16_t h_size;

  /* # of parser and callback functions */
  u_int16_t msg_max;

  /* CPM shutdown message*/
  u_char cpm_server_flags;
  #define CPM_SERVER_SHUTDOWN  (1<<0)

  /* Disconnect callback. */
  CPM_DISCONNECT_CALLBACK disconnect_callback;

  /* Reconnect thread. */
  struct thread *t_connect;

  /* Reconnect interval in seconds. */
  int reconnect_interval;

  /* Debug message flag. */
  int debug;

  /* Service bits. */
  struct cpm_msg_service service;

  /* Callabck called when cpm_client_handler_create() is called */
  cpm_client_handler_cb hccb;

  /* Callabck called when cpm_client_handler_free() is called */
  cpm_client_handler_cb hfcb;

  /* Callabck called when cpm_client_connect() is called */
  cpm_client_handler_cb ccb;

  /* pointer to owner's parser function pointer array */
  CPM_PARSER *parser;

  /* pointer to owner's callback function pointer array */
  CPM_CALLBACK *callback;

  /* Service type pointer which is updated by owner */
  struct cpm_service_type *cpm_service_type;

  /* Maximum service type supported by this specific client */
  int cpm_service_type_max;

#ifdef HAVE_HA
   CPM_HA_CALLBACK ha_callback;
#endif /* HAVE_HA */

#if 0
@@@
@@@ Owner specific
@@@ commsg: HA 
@@@

  /* COMMMSG recv callback. */
  nsm_msg_commsg_recv_cb_t  nc_commsg_recv_cb;
  void                     *nc_commsg_user_ref;

  /* Parser functions. */
  NSM_PARSER parser[NSM_MSG_MAX];

  /* Callback functions. */
  NSM_CALLBACK callback[NSM_MSG_MAX];
#endif/*0*/
};



/*
 * CPM Services Types
 * Each CPM Client will need to define type of services
 * it uses.
 * e.g. For RIB Client
 * struct cpm_service_type rib_service_type[] =
 * {
 *   {RIB_SERVICE_ROUTE,             MESSAGE_TYPE_ASYNC},
 *   {RIB_SERVICE_ROUTE_LOOKUP,      MESSAGE_TYPE_ASYNC},
 * };
 */
struct cpm_service_type
{
  int message;
  int type;
};    

#if 0
@@@ CPM will not know services available. CPM should maintain pointer to 
@@@ start of RIB services. Also services to be mainained in application
@@@ e.g. RIBd
{
  {RIB_SERVICE_ROUTE,             MESSAGE_TYPE_ASYNC},
  {RIB_SERVICE_ROUTE_LOOKUP,      MESSAGE_TYPE_ASYNC},
@@@
@@@ Not RIB services
@@@
  {NSM_SERVICE_IGP_SHORTCUT,      MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_ROUTER_ID,         MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_VRF,               MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_LABEL,             MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_TE,                MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_QOS,               MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_QOS_PREEMPT,       MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_USER,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_SR,                MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_MPLS_VC,           MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_MPLS,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_GMPLS,             MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DIFFSERV,          MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_VPLS,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DSTE,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_MRIB,         MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_PIM,          MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_MCAST_TUNNEL, MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_MRIB,         MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_PIM,          MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_BRIDGE,            MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_VLAN,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_CONTROL_ADJ,       MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_CONTROL_CHANNEL,   MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_TE_LINK,           MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DATA_LINK  ,       MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DATA_LINK_SUB,     MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_GLOBAL_ID_NODE_ID, MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_BGP_VPLS,          MESSAGE_TYPE_ASYNC}
};
#endif/*0*/



/*
 * Public functions
 */
int32_t
cpm_decode_header (u_char **pnt, u_int16_t *size,
                   struct cpm_msg_header *header);
int32_t
cpm_encode_header (u_char **pnt, u_int16_t *size,
                   struct cpm_msg_header *header);
void
cpm_header_dump (struct abc *zg, struct cpm_msg_header *header);

u_int32_t
cpm_owner_id_to_proto_id (u_int32_t cpm_owner_id);

void
cpm_client_id_set (struct cpm_client *cc, struct cpm_msg_service *service);
void
cpm_client_id_unset (struct cpm_client *cc);
void
cpm_client_set_version (struct cpm_client *cc, u_int16_t version);
void
cpm_client_set_protocol (struct cpm_client *cc, u_int32_t protocol_id);
#ifdef HAVE_HA
void
cpm_client_set_ha (struct cpm_client *cc, CPM_HA_CALLBACK ha_callback);
#endif /* HAVE_HA */
void
cpm_client_set_parser (struct cpm_client *cc, int message_type,
                       CPM_PARSER parser);
void
cpm_client_set_callback (struct cpm_client *cc, int message_type,
                         CPM_CALLBACK callback);
void
cpm_client_set_disconnect_callback (struct cpm_client *cc,
                                    CPM_DISCONNECT_CALLBACK callback);
int32_t
cpm_client_set_service (struct cpm_client *cc, int service);
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
                   u_int32_t debug);
int32_t
cpm_client_delete (struct cpm_client *cc);
int32_t
cpm_client_start (struct cpm_client *cc);
void
cpm_client_stop (struct cpm_client *cc);
int32_t
cpm_client_read_msg (struct message_handler *mc,
                     struct message_entry *me,
                     sock_handle_t sock);
int32_t
cpm_client_read_sync (struct message_handler *mc, struct message_entry *me,
                      sock_handle_t sock,
                      struct cpm_msg_header *header, int *type);
void
cpm_client_pending_message (struct cpm_client_handler *cch,
                            struct cpm_msg_header *header);
int32_t
cpm_client_process_pending_msg (struct thread *t);
int32_t
cpm_client_send_message (struct cpm_client_handler *cch,
                         int type, u_int16_t len, u_int32_t *msg_id);

#endif /* __HAVE_CPM_CLIENT_H__ */
