#ifndef QOS_NET_H
#define QOS_NET_H

#include <stdint.h>

#define QOS_NET_MAX_QUEUE 32U
#define QOS_NET_MAX_PACKET 1500U
#define QOS_NET_ARP_MAX 32U
#define QOS_NET_UDP_PORT_MAX 64U
#define QOS_NET_UDP_MAX_DATAGRAMS 64U
#define QOS_NET_TCP_MAX_LISTENERS 64U
#define QOS_NET_TCP_MAX_CONNS 128U

#define QOS_NET_IPV4_LOCAL 0x0A00020FU
#define QOS_NET_IPV4_GATEWAY 0x0A000202U
#define QOS_NET_IPV4_SUBNET_MASK 0xFFFFFF00U
#define QOS_NET_PORT_MIN_DYNAMIC 1024U

enum {
    QOS_TCP_STATE_CLOSED = 0,
    QOS_TCP_STATE_LISTEN = 1,
    QOS_TCP_STATE_SYN_SENT = 2,
    QOS_TCP_STATE_SYN_RECEIVED = 3,
    QOS_TCP_STATE_ESTABLISHED = 4,
    QOS_TCP_STATE_FIN_WAIT_1 = 5,
    QOS_TCP_STATE_FIN_WAIT_2 = 6,
    QOS_TCP_STATE_CLOSE_WAIT = 7,
    QOS_TCP_STATE_CLOSING = 8,
    QOS_TCP_STATE_LAST_ACK = 9,
    QOS_TCP_STATE_TIME_WAIT = 10,
};

enum {
    QOS_TCP_EVT_RX_SYN = 1,
    QOS_TCP_EVT_RX_SYN_ACK = 2,
    QOS_TCP_EVT_RX_ACK = 3,
    QOS_TCP_EVT_APP_CLOSE = 4,
    QOS_TCP_EVT_RX_FIN = 5,
    QOS_TCP_EVT_TIMEOUT = 6,
};

void qos_net_reset(void);
int qos_net_enqueue_packet(const void *data, uint32_t len);
int qos_net_dequeue_packet(void *out, uint32_t cap);
uint32_t qos_net_queue_len(void);
int qos_net_arp_put(uint32_t ip, const uint8_t mac[6], uint32_t ttl_secs);
int qos_net_arp_lookup(uint32_t ip, uint8_t out_mac[6]);
void qos_net_arp_tick(uint32_t elapsed_secs);
uint32_t qos_net_arp_count(void);
int qos_net_ipv4_route(uint32_t dst_ip, uint32_t *out_next_hop);
int qos_net_icmp_echo(uint32_t dst_ip,
                      uint16_t ident,
                      uint16_t seq,
                      const void *payload,
                      uint32_t len,
                      void *out_reply,
                      uint32_t cap,
                      uint32_t *out_src_ip);
int qos_net_udp_bind(uint16_t port);
int qos_net_udp_unbind(uint16_t port);
int qos_net_udp_send(uint16_t src_port, uint32_t src_ip, uint16_t dst_port, const void *data, uint32_t len);
int qos_net_udp_recv(uint16_t dst_port, void *out, uint32_t cap, uint32_t *out_src_ip, uint16_t *out_src_port);
int qos_net_tcp_listen(uint16_t port);
int qos_net_tcp_unlisten(uint16_t port);
int qos_net_tcp_connect(uint16_t local_port, uint32_t remote_ip, uint16_t remote_port);
int qos_net_tcp_step(int conn_id, uint32_t event);
int qos_net_tcp_state(int conn_id);
int qos_net_tcp_rto(int conn_id);
int qos_net_tcp_retries(int conn_id);
uint32_t qos_net_tcp_next_rto_ms(uint32_t current_rto_ms);

void net_init(void);

#endif
