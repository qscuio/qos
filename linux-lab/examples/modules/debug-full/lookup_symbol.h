#ifndef _LOOKUP_SYMBOL_H
#define _LOOKUP_SYMBOL_H

#include <linux/kprobes.h>
#include <linux/version.h>

__maybe_unused static unsigned long lookup_name(const char *name)
{
    struct kprobe kp = {
        .symbol_name = name
    };
    unsigned long retval;

    if (register_kprobe(&kp) < 0) return 0;
    retval = (unsigned long) kp.addr;
    unregister_kprobe(&kp);
    return retval;
}

#endif /* _LOOKUP_SYMBOL_H */ 