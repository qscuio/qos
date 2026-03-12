#include <stddef.h>
#include <string.h>

#include "../init_state.h"
#include "../kernel.h"
#include "drivers.h"

static char g_names[QOS_DRIVERS_MAX][QOS_DRIVER_NAME_MAX];
static uint8_t g_used[QOS_DRIVERS_MAX];
static qos_nic_desc_t g_nics[QOS_DRIVERS_MAX];
static uint8_t g_nic_present[QOS_DRIVERS_MAX];
static uint32_t g_count = 0;

static int name_valid(const char *name) {
    size_t len;
    if (name == NULL) {
        return 0;
    }
    len = strlen(name);
    if (len == 0 || len >= QOS_DRIVER_NAME_MAX) {
        return 0;
    }
    return 1;
}

static int find_name(const char *name) {
    uint32_t i = 0;
    while (i < QOS_DRIVERS_MAX) {
        if (g_used[i] != 0 && strcmp(g_names[i], name) == 0) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

static int free_slot(void) {
    uint32_t i = 0;
    while (i < QOS_DRIVERS_MAX) {
        if (g_used[i] == 0) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

void qos_drivers_reset(void) {
    memset(g_names, 0, sizeof(g_names));
    memset(g_used, 0, sizeof(g_used));
    memset(g_nics, 0, sizeof(g_nics));
    memset(g_nic_present, 0, sizeof(g_nic_present));
    g_count = 0;
}

int qos_drivers_register(const char *name) {
    int slot;

    if (!name_valid(name)) {
        return -1;
    }

    if (find_name(name) >= 0) {
        return -1;
    }

    slot = free_slot();
    if (slot < 0) {
        return -1;
    }

    strcpy(g_names[(uint32_t)slot], name);
    g_used[(uint32_t)slot] = 1;
    g_nic_present[(uint32_t)slot] = 0;
    g_count++;
    return 0;
}

int qos_drivers_loaded(const char *name) {
    if (!name_valid(name)) {
        return 0;
    }
    return find_name(name) >= 0 ? 1 : 0;
}

uint32_t qos_drivers_count(void) {
    return g_count;
}

int qos_drivers_register_nic(const char *name, uint64_t mmio_base, uint32_t irq, uint16_t tx_desc_count, uint16_t rx_desc_count) {
    int slot;
    qos_nic_desc_t *d;

    if (!name_valid(name) || tx_desc_count == 0 || rx_desc_count == 0) {
        return -1;
    }

    if (find_name(name) >= 0) {
        return -1;
    }

    slot = free_slot();
    if (slot < 0) {
        return -1;
    }

    strcpy(g_names[(uint32_t)slot], name);
    g_used[(uint32_t)slot] = 1;
    g_nic_present[(uint32_t)slot] = 1;
    d = &g_nics[(uint32_t)slot];
    d->mmio_base = mmio_base;
    d->irq = irq;
    d->tx_desc_count = tx_desc_count;
    d->rx_desc_count = rx_desc_count;
    d->tx_head = 0;
    d->tx_tail = 0;
    d->rx_head = 0;
    d->rx_tail = 0;
    g_count++;
    return 0;
}

int qos_drivers_get_nic(const char *name, qos_nic_desc_t *out_desc) {
    int idx;
    if (!name_valid(name) || out_desc == NULL) {
        return -1;
    }
    idx = find_name(name);
    if (idx < 0 || g_nic_present[(uint32_t)idx] == 0) {
        return -1;
    }
    *out_desc = g_nics[(uint32_t)idx];
    return 0;
}

int qos_drivers_nic_advance_tx(const char *name, uint16_t produced) {
    int idx;
    qos_nic_desc_t *d;
    if (!name_valid(name)) {
        return -1;
    }
    idx = find_name(name);
    if (idx < 0 || g_nic_present[(uint32_t)idx] == 0) {
        return -1;
    }
    d = &g_nics[(uint32_t)idx];
    if (d->tx_desc_count == 0) {
        return -1;
    }
    d->tx_tail = (uint16_t)((d->tx_tail + produced) % d->tx_desc_count);
    return 0;
}

int qos_drivers_nic_consume_rx(const char *name, uint16_t consumed) {
    int idx;
    qos_nic_desc_t *d;
    if (!name_valid(name)) {
        return -1;
    }
    idx = find_name(name);
    if (idx < 0 || g_nic_present[(uint32_t)idx] == 0) {
        return -1;
    }
    d = &g_nics[(uint32_t)idx];
    if (d->rx_desc_count == 0) {
        return -1;
    }
    d->rx_head = (uint16_t)((d->rx_head + consumed) % d->rx_desc_count);
    return 0;
}

void drivers_init(void) {
    qos_drivers_reset();
    qos_kernel_state_mark(QOS_INIT_DRIVERS);
}
