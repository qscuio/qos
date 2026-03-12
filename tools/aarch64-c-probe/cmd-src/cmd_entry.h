#ifndef QOS_CMD_ENTRY_H
#define QOS_CMD_ENTRY_H

#define QOS_CMD_DECLARE(name_lit) \
    __attribute__((used)) static const char qos_cmd_name[] = name_lit; \
    __attribute__((noreturn)) void _start(void) { \
        for (;;) { \
            __asm__ volatile("" ::: "memory"); \
        } \
    }

#endif
