#include "dummy.h"
#include <string.h>
#include "util.h"

/* Enables support for "dummy" network devices and dpifs, which are useful for
 * testing.  A client program might call this function if it is designed
 * specifically for testing or the user enables it on the command line.
 *
 * 'arg' is parsed to determine the override level (see the definition of enum
 * dummy_level).
 *
 * There is no strong reason why dummy devices shouldn't always be enabled. */
void
dummy_enable(const char *arg)
{
#if 0 /* @liwei */
    enum dummy_level level;

    if (!arg || !arg[0]) {
        level = DUMMY_OVERRIDE_NONE;
    } else if (!strcmp(arg, "system")) {
        level = DUMMY_OVERRIDE_SYSTEM;
    } else if (!strcmp(arg, "override")) {
        level = DUMMY_OVERRIDE_ALL;
    } else {
        ovs_fatal(0, "%s: unknown dummy level", arg);
    }

    netdev_dummy_register(level);
    dpif_dummy_register(level);
    timeval_dummy_register();
    ofpact_dummy_enable();
#endif
}

