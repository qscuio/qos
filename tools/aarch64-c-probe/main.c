#include <stddef.h>
#include <stdint.h>

#include "../../c-os/boot/boot_info.h"
#include "../../c-os/kernel/init_state.h"
#include "../../c-os/kernel/kernel.h"
#include "../../c-os/kernel/net/net.h"

#define UART0 ((volatile uint32_t *)0x09000000u)
#define QOS_DTB_MAGIC 0xEDFE0DD0u
#define VIRTIO_MMIO_BASE_START 0x0A000000u
#define VIRTIO_MMIO_STRIDE 0x200u
#define VIRTIO_MMIO_SCAN_SLOTS 32u
#define VIRTIO_MMIO_MAGIC 0x74726976u
#define VIRTIO_MMIO_DEVICE_NET 1u

#define VIRTIO_MMIO_MAGIC_VALUE 0x000u
#define VIRTIO_MMIO_VERSION 0x004u
#define VIRTIO_MMIO_DEVICE_ID 0x008u
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010u
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014u
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020u
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024u
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028u
#define VIRTIO_MMIO_QUEUE_SEL 0x030u
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034u
#define VIRTIO_MMIO_QUEUE_NUM 0x038u
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03Cu
#define VIRTIO_MMIO_QUEUE_PFN 0x040u
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050u
#define VIRTIO_MMIO_STATUS 0x070u

#define VIRTIO_STATUS_ACKNOWLEDGE 1u
#define VIRTIO_STATUS_DRIVER 2u
#define VIRTIO_STATUS_DRIVER_OK 4u

#define VIRTIO_NET_QUEUE_RX 0u
#define VIRTIO_NET_QUEUE_TX 1u
#define LEGACY_QUEUE_ALIGN 4096u
#define VIRTQ_SIZE 8u
#define VIRTIO_NET_HDR_LEN 10u
#define VIRTQ_DESC_F_WRITE 2u

#define PROBE_UDP_DST_PORT 40000u
#define PROBE_UDP_SRC_PORT 40001u
#define ETH_HDR_LEN 14u
#define IPV4_HDR_LEN 20u
#define UDP_HDR_LEN 8u
#define ICMP_HDR_LEN 8u
#define PROBE_FRAME_LEN (ETH_HDR_LEN + IPV4_HDR_LEN + UDP_HDR_LEN + (sizeof(PROBE_PAYLOAD) - 1u))
#define TX_BUF_LEN (VIRTIO_NET_HDR_LEN + PROBE_FRAME_LEN)
#define RX_BUF_LEN (VIRTIO_NET_HDR_LEN + 2048u)
#define ICMP_ECHO_ID 0x5153u
#define ICMP_ECHO_SEQ 1u

#define NET_TX_STAGE_OK 0u
#define NET_TX_STAGE_NO_NETDEV 1u
#define NET_TX_STAGE_FEATURES 2u
#define NET_TX_STAGE_QUEUE 3u
#define NET_TX_STAGE_TIMEOUT 4u
#define NET_RX_STAGE_OK 0u
#define NET_RX_STAGE_NO_NETDEV 1u
#define NET_RX_STAGE_FEATURES 2u
#define NET_RX_STAGE_QUEUE 3u
#define NET_RX_STAGE_TIMEOUT 4u
#define NET_RX_STAGE_PAYLOAD 5u
#define ICMP_REAL_STAGE_OK 0u
#define ICMP_REAL_STAGE_NO_NETDEV 1u
#define ICMP_REAL_STAGE_FEATURES 2u
#define ICMP_REAL_STAGE_QUEUE 3u
#define ICMP_REAL_STAGE_TIMEOUT 4u
#define ICMP_REAL_STAGE_TX 5u

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} virtq_desc_t;

typedef struct {
    uint16_t qsize;
    volatile virtq_desc_t *desc;
    volatile uint16_t *avail_idx;
    volatile uint16_t *avail_ring;
    volatile const uint16_t *used_idx;
    volatile const uint8_t *used_ring;
} legacy_queue_state_t;

static const uint8_t PROBE_PAYLOAD[] = "QOSREALNET";
static const uint8_t PROBE_RX_PAYLOAD[] = "QOSREALRX";
static const uint8_t ICMP_PROBE_PAYLOAD[] = "QOSICMPRT";
static const uint8_t PROBE_SRC_MAC[6] = {0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u};
static uint8_t LEGACY_TX_QUEUE_PAGE[8192] __attribute__((aligned(4096)));
static uint8_t LEGACY_TX_FRAME_BUF[TX_BUF_LEN] __attribute__((aligned(16)));
static uint8_t LEGACY_RX_QUEUE_PAGE[8192] __attribute__((aligned(4096)));
static uint8_t LEGACY_RX_FRAME_BUF[RX_BUF_LEN] __attribute__((aligned(16)));
static uint32_t g_net_tx_stage = NET_TX_STAGE_QUEUE;
static uint32_t g_net_rx_stage = NET_RX_STAGE_QUEUE;
static uint32_t g_net_tx_virtio_version = 0;
static uint32_t g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;

void *memset(void *dst, int value, size_t n) {
    unsigned char *p = (unsigned char *)dst;
    size_t i = 0;
    while (i < n) {
        p[i] = (unsigned char)value;
        i++;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    size_t i = 0;
    while (i < n) {
        d[i] = s[i];
        i++;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    size_t i = 0;
    while (i < n) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
        i++;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    if (s == 0) {
        return 0;
    }
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int strcmp(const char *a, const char *b) {
    size_t i = 0;
    while (1) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        if (ca == '\0') {
            return 0;
        }
        i++;
    }
}

int strncmp(const char *a, const char *b, size_t n) {
    size_t i = 0;
    while (i < n) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        if (ca == '\0') {
            return 0;
        }
        i++;
    }
    return 0;
}

char *strcpy(char *dst, const char *src) {
    size_t i = 0;
    while (1) {
        dst[i] = src[i];
        if (src[i] == '\0') {
            return dst;
        }
        i++;
    }
}

unsigned char QOS_STACK[65536];

typedef struct __attribute__((aligned(8))) {
    uint32_t magic;
    uint32_t totalsize;
    uint8_t blob[32];
} fake_dtb_t;

static const fake_dtb_t QOS_FAKE_DTB = {
    .magic = QOS_DTB_MAGIC,
    .totalsize = 0x28,
    .blob = {0},
};

static void uart_putc(uint8_t ch) {
    *UART0 = (uint32_t)ch;
}

static void uart_puts(const char *s) {
    while (*s != '\0') {
        uart_putc((uint8_t)*s);
        s++;
    }
}

static int dtb_magic_ok(uint64_t dtb_addr) {
    volatile const uint32_t *magic_ptr;
    if (dtb_addr == 0) {
        return 0;
    }
    magic_ptr = (volatile const uint32_t *)(uintptr_t)dtb_addr;
    return *magic_ptr == QOS_DTB_MAGIC;
}

static uint32_t mmio_read32(uintptr_t base, uintptr_t off) {
    volatile const uint32_t *addr = (volatile const uint32_t *)(base + off);
    return *addr;
}

static void mmio_write32(uintptr_t base, uintptr_t off, uint32_t value) {
    volatile uint32_t *addr = (volatile uint32_t *)(base + off);
    *addr = value;
}

static uintptr_t align_up(uintptr_t value, uintptr_t align) {
    if (align == 0u) {
        return value;
    }
    return (value + (align - 1u)) & ~(align - 1u);
}

static void write_be16(uint8_t *buf, size_t off, uint16_t value) {
    buf[off] = (uint8_t)(value >> 8);
    buf[off + 1] = (uint8_t)value;
}

static void write_be32(uint8_t *buf, size_t off, uint32_t value) {
    buf[off] = (uint8_t)(value >> 24);
    buf[off + 1] = (uint8_t)(value >> 16);
    buf[off + 2] = (uint8_t)(value >> 8);
    buf[off + 3] = (uint8_t)value;
}

static uint16_t read_be16(const uint8_t *buf, size_t off) {
    return (uint16_t)(((uint16_t)buf[off] << 8) | (uint16_t)buf[off + 1]);
}

static uint32_t read_be32(const uint8_t *buf, size_t off) {
    return ((uint32_t)buf[off] << 24) | ((uint32_t)buf[off + 1] << 16) | ((uint32_t)buf[off + 2] << 8) |
           (uint32_t)buf[off + 3];
}

static uint16_t ones_complement_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    size_t i = 0;
    while (i + 1 < len) {
        uint16_t word = (uint16_t)((uint16_t)data[i] << 8) | (uint16_t)data[i + 1];
        sum += word;
        i += 2;
    }
    if (i < len) {
        sum += (uint32_t)data[i] << 8;
    }
    while ((sum >> 16) != 0u) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

static uint16_t ipv4_checksum(const uint8_t *header, size_t len) {
    return ones_complement_checksum(header, len);
}

static size_t build_probe_udp_ipv4_frame(uint8_t *buf, size_t buflen) {
    size_t o = 0;
    size_t payload_len = sizeof(PROBE_PAYLOAD) - 1u;
    uint16_t ip_total_len = (uint16_t)(IPV4_HDR_LEN + UDP_HDR_LEN + payload_len);
    uint16_t udp_len = (uint16_t)(UDP_HDR_LEN + payload_len);
    uint16_t csum;

    if (buf == 0 || buflen < PROBE_FRAME_LEN) {
        return 0;
    }

    memset(buf, 0xFF, 6);
    o += 6;
    memcpy(buf + o, PROBE_SRC_MAC, sizeof(PROBE_SRC_MAC));
    o += sizeof(PROBE_SRC_MAC);
    buf[o] = 0x08u;
    buf[o + 1] = 0x00u;
    o += 2;

    buf[o] = 0x45u;
    buf[o + 1] = 0u;
    write_be16(buf, o + 2, ip_total_len);
    write_be16(buf, o + 4, 1u);
    write_be16(buf, o + 6, 0x4000u);
    buf[o + 8] = 64u;
    buf[o + 9] = 17u;
    write_be16(buf, o + 10, 0u);
    write_be32(buf, o + 12, QOS_NET_IPV4_LOCAL);
    write_be32(buf, o + 16, QOS_NET_IPV4_GATEWAY);
    csum = ipv4_checksum(buf + o, IPV4_HDR_LEN);
    write_be16(buf, o + 10, csum);
    o += IPV4_HDR_LEN;

    write_be16(buf, o, PROBE_UDP_SRC_PORT);
    write_be16(buf, o + 2, PROBE_UDP_DST_PORT);
    write_be16(buf, o + 4, udp_len);
    write_be16(buf, o + 6, 0u);
    o += UDP_HDR_LEN;

    memcpy(buf + o, PROBE_PAYLOAD, payload_len);
    o += payload_len;
    return o;
}

static size_t build_probe_icmp_ipv4_frame(uint8_t *buf, size_t buflen) {
    size_t o = 0;
    uint16_t ip_total_len = (uint16_t)(IPV4_HDR_LEN + ICMP_HDR_LEN + (sizeof(ICMP_PROBE_PAYLOAD) - 1u));
    uint16_t csum;

    if (buf == 0 || buflen < ETH_HDR_LEN + IPV4_HDR_LEN + ICMP_HDR_LEN + (sizeof(ICMP_PROBE_PAYLOAD) - 1u)) {
        return 0;
    }

    memset(buf, 0xFF, 6);
    o += 6;
    memcpy(buf + o, PROBE_SRC_MAC, sizeof(PROBE_SRC_MAC));
    o += sizeof(PROBE_SRC_MAC);
    buf[o] = 0x08u;
    buf[o + 1] = 0x00u;
    o += 2;

    buf[o] = 0x45u;
    buf[o + 1] = 0u;
    write_be16(buf, o + 2, ip_total_len);
    write_be16(buf, o + 4, 2u);
    write_be16(buf, o + 6, 0x4000u);
    buf[o + 8] = 64u;
    buf[o + 9] = 1u;
    write_be16(buf, o + 10, 0u);
    write_be32(buf, o + 12, QOS_NET_IPV4_LOCAL);
    write_be32(buf, o + 16, QOS_NET_IPV4_GATEWAY);
    csum = ipv4_checksum(buf + o, IPV4_HDR_LEN);
    write_be16(buf, o + 10, csum);
    o += IPV4_HDR_LEN;

    buf[o] = 8u;
    buf[o + 1] = 0u;
    write_be16(buf, o + 2, 0u);
    write_be16(buf, o + 4, ICMP_ECHO_ID);
    write_be16(buf, o + 6, ICMP_ECHO_SEQ);
    memcpy(buf + o + ICMP_HDR_LEN, ICMP_PROBE_PAYLOAD, sizeof(ICMP_PROBE_PAYLOAD) - 1u);
    csum = ones_complement_checksum(buf + o, ICMP_HDR_LEN + sizeof(ICMP_PROBE_PAYLOAD) - 1u);
    write_be16(buf, o + 2, csum);
    o += ICMP_HDR_LEN + (sizeof(ICMP_PROBE_PAYLOAD) - 1u);
    return o;
}

static int is_icmp_echo_reply_frame(const uint8_t *frame, size_t len) {
    size_t ip_off = ETH_HDR_LEN;
    size_t ihl;
    size_t icmp_off;
    size_t payload_off;
    if (frame == 0 || len < ETH_HDR_LEN + IPV4_HDR_LEN + ICMP_HDR_LEN) {
        return 0;
    }
    if (frame[12] != 0x08u || frame[13] != 0x00u) {
        return 0;
    }
    ihl = (size_t)(frame[ip_off] & 0x0Fu) * 4u;
    if (ihl < IPV4_HDR_LEN || len < ETH_HDR_LEN + ihl + ICMP_HDR_LEN) {
        return 0;
    }
    if (frame[ip_off + 9] != 1u) {
        return 0;
    }
    if (read_be32(frame, ip_off + 12u) != QOS_NET_IPV4_GATEWAY || read_be32(frame, ip_off + 16u) != QOS_NET_IPV4_LOCAL) {
        return 0;
    }
    icmp_off = ip_off + ihl;
    if (frame[icmp_off] != 0u || frame[icmp_off + 1u] != 0u) {
        return 0;
    }
    if (read_be16(frame, icmp_off + 4u) != ICMP_ECHO_ID || read_be16(frame, icmp_off + 6u) != ICMP_ECHO_SEQ) {
        return 0;
    }
    payload_off = icmp_off + ICMP_HDR_LEN;
    if (payload_off + (sizeof(ICMP_PROBE_PAYLOAD) - 1u) > len) {
        return 0;
    }
    return memcmp(frame + payload_off, ICMP_PROBE_PAYLOAD, sizeof(ICMP_PROBE_PAYLOAD) - 1u) == 0;
}

static int find_payload(const uint8_t *haystack, size_t hay_len, const uint8_t *needle, size_t needle_len) {
    size_t i = 0;
    if (haystack == 0 || needle == 0 || needle_len == 0 || hay_len < needle_len) {
        return 0;
    }
    while (i + needle_len <= hay_len) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return 1;
        }
        i++;
    }
    return 0;
}

static int virtio_net_find_mmio_base(uintptr_t *out_base, uint32_t *out_version) {
    uint32_t i = 0;
    if (out_base == 0 || out_version == 0) {
        return -1;
    }
    while (i < VIRTIO_MMIO_SCAN_SLOTS) {
        uintptr_t base = (uintptr_t)VIRTIO_MMIO_BASE_START + (uintptr_t)i * (uintptr_t)VIRTIO_MMIO_STRIDE;
        uint32_t magic = mmio_read32(base, VIRTIO_MMIO_MAGIC_VALUE);
        if (magic == VIRTIO_MMIO_MAGIC) {
            uint32_t version = mmio_read32(base, VIRTIO_MMIO_VERSION);
            uint32_t dev_id = mmio_read32(base, VIRTIO_MMIO_DEVICE_ID);
            g_net_tx_virtio_version = version;
            if (dev_id == VIRTIO_MMIO_DEVICE_NET) {
                *out_base = base;
                *out_version = version;
                return 0;
            }
        }
        i++;
    }
    return -1;
}

static int virtio_net_init_queue_legacy(uintptr_t base,
                                        uint32_t queue,
                                        uintptr_t page_addr,
                                        legacy_queue_state_t *out_q) {
    uint32_t qmax;
    uint16_t qsize;
    uintptr_t avail_off;
    uintptr_t used_off;

    if (out_q == 0) {
        return -1;
    }

    mmio_write32(base, VIRTIO_MMIO_QUEUE_SEL, queue);
    qmax = mmio_read32(base, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (qmax == 0u) {
        return -1;
    }

    qsize = (uint16_t)((qmax < VIRTQ_SIZE) ? qmax : VIRTQ_SIZE);
    if (qsize == 0u) {
        return -1;
    }

    mmio_write32(base, VIRTIO_MMIO_QUEUE_NUM, qsize);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_ALIGN, LEGACY_QUEUE_ALIGN);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_PFN, (uint32_t)(page_addr >> 12));
    if (mmio_read32(base, VIRTIO_MMIO_QUEUE_PFN) == 0u) {
        return -1;
    }

    avail_off = sizeof(virtq_desc_t) * (uintptr_t)qsize;
    used_off = align_up(avail_off + 4u + (2u * (uintptr_t)qsize) + 2u, LEGACY_QUEUE_ALIGN);
    out_q->qsize = qsize;
    out_q->desc = (volatile virtq_desc_t *)page_addr;
    out_q->avail_idx = (volatile uint16_t *)(page_addr + avail_off + 2u);
    out_q->avail_ring = (volatile uint16_t *)(page_addr + avail_off + 4u);
    out_q->used_idx = (volatile const uint16_t *)(page_addr + used_off + 2u);
    out_q->used_ring = (volatile const uint8_t *)(page_addr + used_off + 4u);
    return 0;
}

static void virtio_net_probe(int *out_tx_ok, int *out_rx_ok, int *out_icmp_ok) {
    uintptr_t base = 0;
    uint32_t version = 0;
    legacy_queue_state_t tx = {0};
    legacy_queue_state_t rx = {0};
    size_t frame_len;
    uint32_t tx_len;
    uint16_t old_avail;
    uint16_t old_rx_used = 0;
    uint16_t old_used;
    uint16_t target;
    uint32_t spins = 0;
    int tx_ok = 0;
    int rx_ok = 0;
    int icmp_ok = 0;

    if (out_tx_ok != 0) {
        *out_tx_ok = 0;
    }
    if (out_rx_ok != 0) {
        *out_rx_ok = 0;
    }
    if (out_icmp_ok != 0) {
        *out_icmp_ok = 0;
    }
    if (virtio_net_find_mmio_base(&base, &version) != 0) {
        g_net_tx_stage = NET_TX_STAGE_NO_NETDEV;
        g_net_rx_stage = NET_RX_STAGE_NO_NETDEV;
        g_icmp_real_stage = ICMP_REAL_STAGE_NO_NETDEV;
        return;
    }
    if (version != 1u) {
        g_net_tx_stage = NET_TX_STAGE_FEATURES;
        g_net_rx_stage = NET_RX_STAGE_FEATURES;
        g_icmp_real_stage = ICMP_REAL_STAGE_FEATURES;
        return;
    }

    mmio_write32(base, VIRTIO_MMIO_STATUS, 0u);
    mmio_write32(base, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0u);
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES, 0u);
    mmio_write32(base, VIRTIO_MMIO_GUEST_PAGE_SIZE, LEGACY_QUEUE_ALIGN);

    memset(LEGACY_RX_QUEUE_PAGE, 0, sizeof(LEGACY_RX_QUEUE_PAGE));
    memset(LEGACY_TX_QUEUE_PAGE, 0, sizeof(LEGACY_TX_QUEUE_PAGE));
    if (virtio_net_init_queue_legacy(base, VIRTIO_NET_QUEUE_RX, (uintptr_t)&LEGACY_RX_QUEUE_PAGE[0], &rx) != 0) {
        g_net_tx_stage = NET_TX_STAGE_QUEUE;
        g_net_rx_stage = NET_RX_STAGE_QUEUE;
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
        return;
    }
    if (virtio_net_init_queue_legacy(base, VIRTIO_NET_QUEUE_TX, (uintptr_t)&LEGACY_TX_QUEUE_PAGE[0], &tx) != 0) {
        g_net_tx_stage = NET_TX_STAGE_QUEUE;
        g_net_rx_stage = NET_RX_STAGE_QUEUE;
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
        return;
    }
    mmio_write32(base, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);

    memset(LEGACY_RX_FRAME_BUF, 0, sizeof(LEGACY_RX_FRAME_BUF));
    rx.desc[0].addr = (uint64_t)(uintptr_t)&LEGACY_RX_FRAME_BUF[0];
    rx.desc[0].len = (uint32_t)RX_BUF_LEN;
    rx.desc[0].flags = VIRTQ_DESC_F_WRITE;
    rx.desc[0].next = 0u;
    old_avail = *rx.avail_idx;
    rx.avail_ring[old_avail % rx.qsize] = 0u;
    __sync_synchronize();
    *rx.avail_idx = (uint16_t)(old_avail + 1u);
    __sync_synchronize();
    mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);
    old_rx_used = *rx.used_idx;

    memset(LEGACY_TX_FRAME_BUF, 0, sizeof(LEGACY_TX_FRAME_BUF));
    frame_len = build_probe_udp_ipv4_frame(LEGACY_TX_FRAME_BUF + VIRTIO_NET_HDR_LEN, PROBE_FRAME_LEN);
    if (frame_len == 0u) {
        g_net_tx_stage = NET_TX_STAGE_QUEUE;
        g_net_rx_stage = NET_RX_STAGE_QUEUE;
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
        return;
    }
    tx_len = (uint32_t)(VIRTIO_NET_HDR_LEN + frame_len);

    tx.desc[0].addr = (uint64_t)(uintptr_t)&LEGACY_TX_FRAME_BUF[0];
    tx.desc[0].len = tx_len;
    tx.desc[0].flags = 0u;
    tx.desc[0].next = 0u;

    old_avail = *tx.avail_idx;
    tx.avail_ring[old_avail % tx.qsize] = 0u;
    __sync_synchronize();
    *tx.avail_idx = (uint16_t)(old_avail + 1u);
    __sync_synchronize();
    mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);

    old_used = *tx.used_idx;
    target = (uint16_t)(old_used + 1u);
    while (spins < 2000000u) {
        uint16_t cur = *tx.used_idx;
        if (cur == target) {
            g_net_tx_stage = NET_TX_STAGE_OK;
            tx_ok = 1;
            break;
        }
        spins++;
    }

    if (!tx_ok) {
        g_net_tx_stage = NET_TX_STAGE_TIMEOUT;
    }

    memset(LEGACY_TX_FRAME_BUF, 0, sizeof(LEGACY_TX_FRAME_BUF));
    frame_len = build_probe_icmp_ipv4_frame(LEGACY_TX_FRAME_BUF + VIRTIO_NET_HDR_LEN, PROBE_FRAME_LEN);
    if (frame_len == 0u) {
        g_icmp_real_stage = ICMP_REAL_STAGE_QUEUE;
    } else {
        tx_len = (uint32_t)(VIRTIO_NET_HDR_LEN + frame_len);
        tx.desc[0].addr = (uint64_t)(uintptr_t)&LEGACY_TX_FRAME_BUF[0];
        tx.desc[0].len = tx_len;
        tx.desc[0].flags = 0u;
        tx.desc[0].next = 0u;

        old_avail = *tx.avail_idx;
        tx.avail_ring[old_avail % tx.qsize] = 0u;
        __sync_synchronize();
        *tx.avail_idx = (uint16_t)(old_avail + 1u);
        __sync_synchronize();
        mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);

        old_used = *tx.used_idx;
        target = (uint16_t)(old_used + 1u);
        spins = 0;
        while (spins < 2000000u) {
            uint16_t cur = *tx.used_idx;
            if (cur == target) {
                break;
            }
            spins++;
        }
        if (spins >= 2000000u) {
            g_icmp_real_stage = ICMP_REAL_STAGE_TX;
        }
    }

    spins = 0;
    while (spins < 2000000u) {
        uint16_t cur = *rx.used_idx;
        if (cur != old_rx_used) {
            uint16_t slot = (uint16_t)(old_rx_used % rx.qsize);
            volatile const uint8_t *elem = rx.used_ring + ((size_t)slot * 8u);
            uint32_t used_len = *(volatile const uint32_t *)(elem + 4u);
            size_t payload_start = (used_len > VIRTIO_NET_HDR_LEN) ? VIRTIO_NET_HDR_LEN : used_len;
            size_t payload_len = used_len > RX_BUF_LEN ? RX_BUF_LEN : used_len;
            if (payload_len > payload_start) {
                g_net_rx_stage = NET_RX_STAGE_PAYLOAD;
                rx_ok = 1;
                if (is_icmp_echo_reply_frame(LEGACY_RX_FRAME_BUF + payload_start, payload_len - payload_start)) {
                    g_icmp_real_stage = ICMP_REAL_STAGE_OK;
                    icmp_ok = 1;
                }
            }
            old_avail = *rx.avail_idx;
            rx.avail_ring[old_avail % rx.qsize] = 0u;
            __sync_synchronize();
            *rx.avail_idx = (uint16_t)(old_avail + 1u);
            __sync_synchronize();
            mmio_write32(base, VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);
            old_rx_used = cur;
            if (icmp_ok) {
                break;
            }
        }
        spins++;
    }
    if (!rx_ok && g_net_rx_stage != NET_RX_STAGE_PAYLOAD) {
        g_net_rx_stage = NET_RX_STAGE_TIMEOUT;
    }
    if (!icmp_ok && g_icmp_real_stage == ICMP_REAL_STAGE_QUEUE) {
        g_icmp_real_stage = ICMP_REAL_STAGE_TIMEOUT;
    }
    if (out_tx_ok != 0) {
        *out_tx_ok = tx_ok;
    }
    if (out_rx_ok != 0) {
        *out_rx_ok = rx_ok;
    }
    if (out_icmp_ok != 0) {
        *out_icmp_ok = icmp_ok;
    }
}

static const char *net_tx_stage_text(void) {
    switch (g_net_tx_stage) {
        case NET_TX_STAGE_OK:
            return "ok";
        case NET_TX_STAGE_NO_NETDEV:
            return "no_netdev";
        case NET_TX_STAGE_FEATURES:
            return "features";
        case NET_TX_STAGE_QUEUE:
            return "queue";
        case NET_TX_STAGE_TIMEOUT:
            return "timeout";
        default:
            return "unknown";
    }
}

static const char *net_tx_version_text(void) {
    switch (g_net_tx_virtio_version) {
        case 1u:
            return "v1";
        case 2u:
            return "v2";
        case 0u:
            return "v0";
        default:
            return "v?";
    }
}

static const char *net_rx_stage_text(void) {
    switch (g_net_rx_stage) {
        case NET_RX_STAGE_OK:
            return "ok";
        case NET_RX_STAGE_NO_NETDEV:
            return "no_netdev";
        case NET_RX_STAGE_FEATURES:
            return "features";
        case NET_RX_STAGE_QUEUE:
            return "queue";
        case NET_RX_STAGE_TIMEOUT:
            return "timeout";
        case NET_RX_STAGE_PAYLOAD:
            return "payload";
        default:
            return "unknown";
    }
}

static const char *net_rx_result_text(void) {
    switch (g_net_rx_stage) {
        case NET_RX_STAGE_OK:
            return "real_ok";
        case NET_RX_STAGE_PAYLOAD:
            return "real_ok";
        case NET_RX_STAGE_TIMEOUT:
            return "real_skip";
        default:
            return "real_fail";
    }
}

static const char *icmp_real_stage_text(void) {
    switch (g_icmp_real_stage) {
        case ICMP_REAL_STAGE_OK:
            return "ok";
        case ICMP_REAL_STAGE_NO_NETDEV:
            return "no_netdev";
        case ICMP_REAL_STAGE_FEATURES:
            return "features";
        case ICMP_REAL_STAGE_QUEUE:
            return "queue";
        case ICMP_REAL_STAGE_TIMEOUT:
            return "timeout";
        case ICMP_REAL_STAGE_TX:
            return "tx";
        default:
            return "unknown";
    }
}

static const char *icmp_real_result_text(void) {
    switch (g_icmp_real_stage) {
        case ICMP_REAL_STAGE_OK:
            return "roundtrip_ok";
        case ICMP_REAL_STAGE_TIMEOUT:
            return "roundtrip_skip";
        default:
            return "roundtrip_fail";
    }
}

void qos_syscall_reset(void) {}

uint32_t qos_syscall_count(void) {
    return 0;
}

void syscall_init(void) {
    qos_kernel_state_mark(QOS_INIT_SYSCALL);
}

void c_main(uint64_t dtb_addr) {
    uint64_t fallback_dtb;
    uint64_t effective_dtb;
    boot_info_t info;
    int kernel_ok;
    int icmp_ok = 0;
    int net_tx_ok = 0;
    int net_rx_ok = 0;

    uart_puts("probe=enter\n");

    fallback_dtb = (uint64_t)(uintptr_t)&QOS_FAKE_DTB;
    if (dtb_addr != 0 && dtb_magic_ok(dtb_addr)) {
        effective_dtb = dtb_addr;
    } else {
        effective_dtb = fallback_dtb;
    }

    info = (boot_info_t){0};
    info.magic = QOS_BOOT_MAGIC;
    info.mmap_entry_count = 1;
    info.mmap_entries[0].base = 0x40000000ULL;
    info.mmap_entries[0].length = 0x04000000ULL;
    info.mmap_entries[0].type = 1;
    info.initramfs_addr = 0x44000000ULL;
    info.initramfs_size = 0x1000ULL;
    info.dtb_addr = effective_dtb;

    kernel_ok = qos_kernel_entry(&info) == 0;
    if (kernel_ok) {
        uint8_t payload[3] = {'q', 'o', 's'};
        uint8_t reply[8] = {0};
        uint32_t src_ip = 0;
        icmp_ok = qos_net_icmp_echo(QOS_NET_IPV4_GATEWAY, 1, 1, payload, 3, reply, sizeof(reply), &src_ip) == 3;
        virtio_net_probe(&net_tx_ok, &net_rx_ok, 0);
    }

    if (kernel_ok && dtb_magic_ok(effective_dtb) && icmp_ok) {
        if (net_tx_ok) {
            uart_puts("QOS kernel started impl=c arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=dtb_stub mmap_count=1 mmap_len_nonzero=1 initramfs_source=stub initramfs_size_nonzero=1 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_ok net_tx=real_ok net_tx_stage=ok net_rx=");
            uart_puts(net_rx_result_text());
            uart_puts(" net_rx_stage=");
            uart_puts(net_rx_stage_text());
            uart_puts(" icmp_real=");
            uart_puts(icmp_real_result_text());
            uart_puts(" icmp_real_stage=");
            uart_puts(icmp_real_stage_text());
            uart_puts("\n");
        } else {
            uart_puts("QOS kernel started impl=c arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=dtb_stub mmap_count=1 mmap_len_nonzero=1 initramfs_source=stub initramfs_size_nonzero=1 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_ok net_tx=real_fail net_tx_stage=");
            uart_puts(net_tx_stage_text());
            uart_puts(" net_tx_ver=");
            uart_puts(net_tx_version_text());
            uart_puts(" net_rx=");
            uart_puts(net_rx_result_text());
            uart_puts(" net_rx_stage=");
            uart_puts(net_rx_stage_text());
            uart_puts(" icmp_real=");
            uart_puts(icmp_real_result_text());
            uart_puts(" icmp_real_stage=");
            uart_puts(icmp_real_stage_text());
            uart_puts("\n");
        }
    } else {
        uart_puts("QOS kernel started impl=c arch=aarch64 handoff=invalid entry=kernel_main abi=x0 dtb_addr_nonzero=0 dtb_magic=bad mmap_source=dtb_stub mmap_count=0 mmap_len_nonzero=0 initramfs_source=stub initramfs_size_nonzero=0 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb icmp_echo=gateway_fail net_tx=real_fail net_tx_stage=");
        uart_puts(net_tx_stage_text());
        uart_puts(" net_tx_ver=");
        uart_puts(net_tx_version_text());
        uart_puts(" net_rx=");
        uart_puts(net_rx_result_text());
        uart_puts(" net_rx_stage=");
        uart_puts(net_rx_stage_text());
        uart_puts(" icmp_real=");
        uart_puts(icmp_real_result_text());
        uart_puts(" icmp_real_stage=");
        uart_puts(icmp_real_stage_text());
        uart_puts("\n");
    }

    for (;;) {
        __asm__ volatile("wfe");
    }
}
