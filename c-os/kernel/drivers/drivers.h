#ifndef QOS_DRIVERS_H
#define QOS_DRIVERS_H

#include <stdint.h>

#define QOS_DRIVERS_MAX 64U
#define QOS_DRIVER_NAME_MAX 64U

typedef struct {
    uint64_t mmio_base;
    uint32_t irq;
    uint16_t tx_desc_count;
    uint16_t rx_desc_count;
    uint16_t tx_head;
    uint16_t tx_tail;
    uint16_t rx_head;
    uint16_t rx_tail;
} qos_nic_desc_t;

void qos_drivers_reset(void);
int qos_drivers_register(const char *name);
int qos_drivers_loaded(const char *name);
uint32_t qos_drivers_count(void);
int qos_drivers_register_nic(const char *name, uint64_t mmio_base, uint32_t irq, uint16_t tx_desc_count, uint16_t rx_desc_count);
int qos_drivers_get_nic(const char *name, qos_nic_desc_t *out_desc);
int qos_drivers_nic_advance_tx(const char *name, uint16_t produced);
int qos_drivers_nic_consume_rx(const char *name, uint16_t consumed);

void drivers_init(void);

#endif
