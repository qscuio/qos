#include <stdint.h>

__attribute__((visibility("default"))) uint32_t module_init(void) {
    return 0x51534D4Fu;
}

__attribute__((visibility("default"))) uint32_t module_tick(uint32_t value) {
    return value + 1u;
}
