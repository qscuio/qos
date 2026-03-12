#include "init_state.h"

static uint32_t g_init_state = 0;

void qos_kernel_state_reset(void) {
    g_init_state = 0;
}

void qos_kernel_state_mark(uint32_t bit) {
    g_init_state |= bit;
}

uint32_t qos_kernel_state_get(void) {
    return g_init_state;
}
