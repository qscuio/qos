#include <stdint.h>

__attribute__((visibility("default"))) uint32_t plugin_init(void) {
    return 0x51534C49u;
}

__attribute__((visibility("default"))) const char *plugin_name(void) {
    return "qos_testlib";
}
