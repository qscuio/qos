#include <stddef.h>
#include <string.h>

#include "../init_state.h"
#include "../kernel.h"
#include "net.h"

static unsigned char g_packets[QOS_NET_MAX_QUEUE][QOS_NET_MAX_PACKET];
static uint16_t g_lens[QOS_NET_MAX_QUEUE];
static uint32_t g_head = 0;
static uint32_t g_tail = 0;
static uint32_t g_count = 0;

typedef struct {
    uint8_t used;
    uint32_t ip;
    uint8_t mac[6];
    uint32_t ttl_secs;
} arp_entry_t;

typedef struct {
    uint8_t used;
    uint16_t port;
} udp_port_t;

typedef struct {
    uint8_t used;
    uint16_t dst_port;
    uint16_t src_port;
    uint32_t src_ip;
    uint16_t len;
    uint8_t data[QOS_NET_MAX_PACKET];
} udp_datagram_t;

typedef struct {
    uint8_t used;
    uint8_t state;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t remote_ip;
    uint32_t rto_ms;
    uint8_t retries;
} tcp_conn_t;

static arp_entry_t g_arp[QOS_NET_ARP_MAX];
static udp_port_t g_udp_ports[QOS_NET_UDP_PORT_MAX];
static udp_datagram_t g_udp_datagrams[QOS_NET_UDP_MAX_DATAGRAMS];
static uint32_t g_udp_count = 0;
static uint16_t g_tcp_listeners[QOS_NET_TCP_MAX_LISTENERS];
static uint8_t g_tcp_listener_used[QOS_NET_TCP_MAX_LISTENERS];
static tcp_conn_t g_tcp_conns[QOS_NET_TCP_MAX_CONNS];

static int same_subnet(uint32_t a, uint32_t b) {
    return (a & QOS_NET_IPV4_SUBNET_MASK) == (b & QOS_NET_IPV4_SUBNET_MASK);
}

static int udp_port_is_bound(uint16_t port) {
    uint32_t i = 0;
    while (i < QOS_NET_UDP_PORT_MAX) {
        if (g_udp_ports[i].used != 0 && g_udp_ports[i].port == port) {
            return 1;
        }
        i++;
    }
    return 0;
}

static void udp_drop_port_datagrams(uint16_t port) {
    uint32_t src = 0;
    uint32_t dst = 0;
    while (src < g_udp_count) {
        if (g_udp_datagrams[src].used != 0 && g_udp_datagrams[src].dst_port == port) {
            src++;
            continue;
        }
        if (dst != src) {
            g_udp_datagrams[dst] = g_udp_datagrams[src];
        }
        dst++;
        src++;
    }
    while (dst < g_udp_count) {
        memset(&g_udp_datagrams[dst], 0, sizeof(g_udp_datagrams[dst]));
        dst++;
    }
    g_udp_count = dst;
}

static int tcp_listener_exists(uint16_t port) {
    uint32_t i = 0;
    while (i < QOS_NET_TCP_MAX_LISTENERS) {
        if (g_tcp_listener_used[i] != 0 && g_tcp_listeners[i] == port) {
            return 1;
        }
        i++;
    }
    return 0;
}

static uint32_t tcp_next_rto(uint32_t rto_ms) {
    if (rto_ms >= 60000U) {
        return 60000U;
    }
    if (rto_ms > 30000U) {
        return 60000U;
    }
    return rto_ms * 2U;
}

void qos_net_reset(void) {
    memset(g_packets, 0, sizeof(g_packets));
    memset(g_lens, 0, sizeof(g_lens));
    g_head = 0;
    g_tail = 0;
    g_count = 0;
    memset(g_arp, 0, sizeof(g_arp));
    memset(g_udp_ports, 0, sizeof(g_udp_ports));
    memset(g_udp_datagrams, 0, sizeof(g_udp_datagrams));
    g_udp_count = 0;
    memset(g_tcp_listeners, 0, sizeof(g_tcp_listeners));
    memset(g_tcp_listener_used, 0, sizeof(g_tcp_listener_used));
    memset(g_tcp_conns, 0, sizeof(g_tcp_conns));
}

int qos_net_enqueue_packet(const void *data, uint32_t len) {
    if (data == NULL || len == 0 || len > QOS_NET_MAX_PACKET) {
        return -1;
    }

    if (g_count >= QOS_NET_MAX_QUEUE) {
        return -1;
    }

    memcpy(g_packets[g_tail], data, len);
    g_lens[g_tail] = (uint16_t)len;
    g_tail = (g_tail + 1U) % QOS_NET_MAX_QUEUE;
    g_count++;
    return 0;
}

int qos_net_dequeue_packet(void *out, uint32_t cap) {
    uint16_t len;

    if (out == NULL || cap == 0) {
        return -1;
    }

    if (g_count == 0) {
        return -1;
    }

    len = g_lens[g_head];
    if (cap < len) {
        return -1;
    }

    memcpy(out, g_packets[g_head], len);
    g_lens[g_head] = 0;
    g_head = (g_head + 1U) % QOS_NET_MAX_QUEUE;
    g_count--;
    return (int)len;
}

uint32_t qos_net_queue_len(void) {
    return g_count;
}

int qos_net_arp_put(uint32_t ip, const uint8_t mac[6], uint32_t ttl_secs) {
    uint32_t i = 0;
    int free_idx = -1;
    if (ip == 0 || mac == NULL || ttl_secs == 0) {
        return -1;
    }
    while (i < QOS_NET_ARP_MAX) {
        if (g_arp[i].used != 0 && g_arp[i].ip == ip) {
            memcpy(g_arp[i].mac, mac, 6);
            g_arp[i].ttl_secs = ttl_secs;
            return 0;
        }
        if (free_idx < 0 && g_arp[i].used == 0) {
            free_idx = (int)i;
        }
        i++;
    }
    if (free_idx < 0) {
        return -1;
    }
    g_arp[(uint32_t)free_idx].used = 1;
    g_arp[(uint32_t)free_idx].ip = ip;
    memcpy(g_arp[(uint32_t)free_idx].mac, mac, 6);
    g_arp[(uint32_t)free_idx].ttl_secs = ttl_secs;
    return 0;
}

int qos_net_arp_lookup(uint32_t ip, uint8_t out_mac[6]) {
    uint32_t i = 0;
    if (ip == 0 || out_mac == NULL) {
        return -1;
    }
    while (i < QOS_NET_ARP_MAX) {
        if (g_arp[i].used != 0 && g_arp[i].ip == ip && g_arp[i].ttl_secs != 0) {
            memcpy(out_mac, g_arp[i].mac, 6);
            return 0;
        }
        i++;
    }
    return -1;
}

void qos_net_arp_tick(uint32_t elapsed_secs) {
    uint32_t i = 0;
    if (elapsed_secs == 0) {
        return;
    }
    while (i < QOS_NET_ARP_MAX) {
        if (g_arp[i].used != 0) {
            if (g_arp[i].ttl_secs <= elapsed_secs) {
                memset(&g_arp[i], 0, sizeof(g_arp[i]));
            } else {
                g_arp[i].ttl_secs -= elapsed_secs;
            }
        }
        i++;
    }
}

uint32_t qos_net_arp_count(void) {
    uint32_t i = 0;
    uint32_t count = 0;
    while (i < QOS_NET_ARP_MAX) {
        if (g_arp[i].used != 0) {
            count++;
        }
        i++;
    }
    return count;
}

int qos_net_ipv4_route(uint32_t dst_ip, uint32_t *out_next_hop) {
    if (dst_ip == 0 || out_next_hop == NULL) {
        return -1;
    }
    if (same_subnet(dst_ip, QOS_NET_IPV4_LOCAL)) {
        *out_next_hop = dst_ip;
    } else {
        *out_next_hop = QOS_NET_IPV4_GATEWAY;
    }
    return 0;
}

int qos_net_icmp_echo(uint32_t dst_ip,
                      uint16_t ident,
                      uint16_t seq,
                      const void *payload,
                      uint32_t len,
                      void *out_reply,
                      uint32_t cap,
                      uint32_t *out_src_ip) {
    uint32_t next_hop = 0;
    (void)ident;
    (void)seq;
    if (dst_ip == 0 || payload == NULL || out_reply == NULL || len == 0 || len > QOS_NET_MAX_PACKET || cap < len) {
        return -1;
    }
    if (qos_net_ipv4_route(dst_ip, &next_hop) != 0) {
        return -1;
    }
    if (dst_ip != QOS_NET_IPV4_GATEWAY && dst_ip != QOS_NET_IPV4_LOCAL) {
        return -1;
    }
    memcpy(out_reply, payload, len);
    if (out_src_ip != NULL) {
        *out_src_ip = dst_ip;
    }
    return (int)len;
}

int qos_net_udp_bind(uint16_t port) {
    uint32_t i = 0;
    if (port < QOS_NET_PORT_MIN_DYNAMIC || udp_port_is_bound(port) != 0) {
        return -1;
    }
    while (i < QOS_NET_UDP_PORT_MAX) {
        if (g_udp_ports[i].used == 0) {
            g_udp_ports[i].used = 1;
            g_udp_ports[i].port = port;
            return 0;
        }
        i++;
    }
    return -1;
}

int qos_net_udp_unbind(uint16_t port) {
    uint32_t i = 0;
    while (i < QOS_NET_UDP_PORT_MAX) {
        if (g_udp_ports[i].used != 0 && g_udp_ports[i].port == port) {
            g_udp_ports[i].used = 0;
            g_udp_ports[i].port = 0;
            udp_drop_port_datagrams(port);
            return 0;
        }
        i++;
    }
    return -1;
}

int qos_net_udp_send(uint16_t src_port, uint32_t src_ip, uint16_t dst_port, const void *data, uint32_t len) {
    udp_datagram_t *d;
    int dst_is_local;
    if (src_port < QOS_NET_PORT_MIN_DYNAMIC || src_ip == 0 || dst_port == 0 || data == NULL || len == 0 ||
        len > QOS_NET_MAX_PACKET) {
        return -1;
    }
    dst_is_local = udp_port_is_bound(dst_port);
    if (dst_is_local == 0) {
        return (int)len;
    }
    if (g_udp_count >= QOS_NET_UDP_MAX_DATAGRAMS) {
        return -1;
    }
    d = &g_udp_datagrams[g_udp_count];
    d->used = 1;
    d->dst_port = dst_port;
    d->src_port = src_port;
    d->src_ip = src_ip;
    d->len = (uint16_t)len;
    memcpy(d->data, data, len);
    g_udp_count++;
    return (int)len;
}

int qos_net_udp_recv(uint16_t dst_port, void *out, uint32_t cap, uint32_t *out_src_ip, uint16_t *out_src_port) {
    uint32_t i = 0;
    if (dst_port == 0 || out == NULL || cap == 0) {
        return -1;
    }
    while (i < g_udp_count) {
        if (g_udp_datagrams[i].used != 0 && g_udp_datagrams[i].dst_port == dst_port) {
            uint16_t len = g_udp_datagrams[i].len;
            uint32_t j;
            if (cap < len) {
                return -1;
            }
            memcpy(out, g_udp_datagrams[i].data, len);
            if (out_src_ip != NULL) {
                *out_src_ip = g_udp_datagrams[i].src_ip;
            }
            if (out_src_port != NULL) {
                *out_src_port = g_udp_datagrams[i].src_port;
            }
            j = i;
            while (j + 1U < g_udp_count) {
                g_udp_datagrams[j] = g_udp_datagrams[j + 1U];
                j++;
            }
            memset(&g_udp_datagrams[g_udp_count - 1U], 0, sizeof(g_udp_datagrams[0]));
            g_udp_count--;
            return (int)len;
        }
        i++;
    }
    return -1;
}

int qos_net_tcp_listen(uint16_t port) {
    uint32_t i = 0;
    if (port < QOS_NET_PORT_MIN_DYNAMIC || tcp_listener_exists(port) != 0) {
        return -1;
    }
    while (i < QOS_NET_TCP_MAX_LISTENERS) {
        if (g_tcp_listener_used[i] == 0) {
            g_tcp_listener_used[i] = 1;
            g_tcp_listeners[i] = port;
            return (int)i;
        }
        i++;
    }
    return -1;
}

int qos_net_tcp_unlisten(uint16_t port) {
    uint32_t i = 0;
    while (i < QOS_NET_TCP_MAX_LISTENERS) {
        if (g_tcp_listener_used[i] != 0 && g_tcp_listeners[i] == port) {
            g_tcp_listener_used[i] = 0;
            g_tcp_listeners[i] = 0;
            return 0;
        }
        i++;
    }
    return -1;
}

int qos_net_tcp_connect(uint16_t local_port, uint32_t remote_ip, uint16_t remote_port) {
    uint32_t i = 0;
    if (local_port < QOS_NET_PORT_MIN_DYNAMIC || remote_ip == 0 || remote_port == 0 ||
        tcp_listener_exists(remote_port) == 0) {
        return -1;
    }
    while (i < QOS_NET_TCP_MAX_CONNS) {
        if (g_tcp_conns[i].used == 0) {
            g_tcp_conns[i].used = 1;
            g_tcp_conns[i].state = QOS_TCP_STATE_SYN_SENT;
            g_tcp_conns[i].local_port = local_port;
            g_tcp_conns[i].remote_port = remote_port;
            g_tcp_conns[i].remote_ip = remote_ip;
            g_tcp_conns[i].rto_ms = 500U;
            g_tcp_conns[i].retries = 0;
            return (int)i;
        }
        i++;
    }
    return -1;
}

int qos_net_tcp_step(int conn_id, uint32_t event) {
    tcp_conn_t *conn;
    if (conn_id < 0 || (uint32_t)conn_id >= QOS_NET_TCP_MAX_CONNS || g_tcp_conns[(uint32_t)conn_id].used == 0) {
        return -1;
    }
    conn = &g_tcp_conns[(uint32_t)conn_id];
    if (event == QOS_TCP_EVT_TIMEOUT) {
        if (conn->state == QOS_TCP_STATE_TIME_WAIT) {
            conn->state = QOS_TCP_STATE_CLOSED;
            return 0;
        }
        if (conn->state == QOS_TCP_STATE_SYN_SENT) {
            if (conn->retries >= 5U) {
                conn->state = QOS_TCP_STATE_CLOSED;
            } else {
                conn->retries++;
                conn->rto_ms = tcp_next_rto(conn->rto_ms);
            }
            return 0;
        }
    }

    switch (conn->state) {
        case QOS_TCP_STATE_LISTEN:
            if (event == QOS_TCP_EVT_RX_SYN) {
                conn->state = QOS_TCP_STATE_SYN_RECEIVED;
                return 0;
            }
            break;
        case QOS_TCP_STATE_SYN_SENT:
            if (event == QOS_TCP_EVT_RX_SYN_ACK) {
                conn->state = QOS_TCP_STATE_ESTABLISHED;
                return 0;
            }
            break;
        case QOS_TCP_STATE_SYN_RECEIVED:
            if (event == QOS_TCP_EVT_RX_ACK) {
                conn->state = QOS_TCP_STATE_ESTABLISHED;
                return 0;
            }
            break;
        case QOS_TCP_STATE_ESTABLISHED:
            if (event == QOS_TCP_EVT_APP_CLOSE) {
                conn->state = QOS_TCP_STATE_FIN_WAIT_1;
                return 0;
            }
            if (event == QOS_TCP_EVT_RX_FIN) {
                conn->state = QOS_TCP_STATE_CLOSE_WAIT;
                return 0;
            }
            break;
        case QOS_TCP_STATE_FIN_WAIT_1:
            if (event == QOS_TCP_EVT_RX_ACK) {
                conn->state = QOS_TCP_STATE_FIN_WAIT_2;
                return 0;
            }
            if (event == QOS_TCP_EVT_RX_FIN) {
                conn->state = QOS_TCP_STATE_CLOSING;
                return 0;
            }
            break;
        case QOS_TCP_STATE_FIN_WAIT_2:
            if (event == QOS_TCP_EVT_RX_FIN) {
                conn->state = QOS_TCP_STATE_TIME_WAIT;
                return 0;
            }
            break;
        case QOS_TCP_STATE_CLOSE_WAIT:
            if (event == QOS_TCP_EVT_APP_CLOSE) {
                conn->state = QOS_TCP_STATE_LAST_ACK;
                return 0;
            }
            break;
        case QOS_TCP_STATE_CLOSING:
            if (event == QOS_TCP_EVT_RX_ACK) {
                conn->state = QOS_TCP_STATE_TIME_WAIT;
                return 0;
            }
            break;
        case QOS_TCP_STATE_LAST_ACK:
            if (event == QOS_TCP_EVT_RX_ACK) {
                conn->state = QOS_TCP_STATE_CLOSED;
                return 0;
            }
            break;
        default:
            break;
    }
    return -1;
}

int qos_net_tcp_state(int conn_id) {
    if (conn_id < 0 || (uint32_t)conn_id >= QOS_NET_TCP_MAX_CONNS || g_tcp_conns[(uint32_t)conn_id].used == 0) {
        return -1;
    }
    return (int)g_tcp_conns[(uint32_t)conn_id].state;
}

int qos_net_tcp_rto(int conn_id) {
    if (conn_id < 0 || (uint32_t)conn_id >= QOS_NET_TCP_MAX_CONNS || g_tcp_conns[(uint32_t)conn_id].used == 0) {
        return -1;
    }
    return (int)g_tcp_conns[(uint32_t)conn_id].rto_ms;
}

int qos_net_tcp_retries(int conn_id) {
    if (conn_id < 0 || (uint32_t)conn_id >= QOS_NET_TCP_MAX_CONNS || g_tcp_conns[(uint32_t)conn_id].used == 0) {
        return -1;
    }
    return (int)g_tcp_conns[(uint32_t)conn_id].retries;
}

uint32_t qos_net_tcp_next_rto_ms(uint32_t current_rto_ms) {
    return tcp_next_rto(current_rto_ms);
}

void net_init(void) {
    qos_net_reset();
    qos_kernel_state_mark(QOS_INIT_NET);
}
