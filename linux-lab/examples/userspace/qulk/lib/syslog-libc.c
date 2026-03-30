#include "syslog-libc.h"
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include "compiler.h"
#include "dynamic-string.h"
#include "socket-util.h"
#include "syslog-provider.h"
#include "util.h"


static void syslog_libc_open(struct syslogger *this, int facility);
static void syslog_libc_log(struct syslogger *this, int pri, const char *msg);

static struct syslog_class syslog_libc_class = {
    syslog_libc_open,
    syslog_libc_log,
};

struct syslog_libc {
    struct syslogger parent;
};


/* This function  creates object that delegate all logging to libc's
 * syslog implementation. */
struct syslogger *
syslog_libc_create(void)
{
    struct syslog_libc *this = xmalloc(sizeof *this);

    this->parent.class = &syslog_libc_class;
    this->parent.prefix = "<%B> %D{%h %e %T} %E %A:";

    return &this->parent;
}

static void
syslog_libc_open(struct syslogger *this OVS_UNUSED, int facility)
{
    static char *ident;

    /* openlog() is allowed to keep the pointer passed in, without making a
     * copy.  The daemonize code sometimes frees and replaces
     * 'program_name', so make a private copy just for openlog().  (We keep
     * a pointer to the private copy to suppress memory leak warnings in
     * case openlog() does make its own copy.) */
    ident = nullable_xstrdup(program_name);

    openlog(ident, LOG_NDELAY, facility);
}

static void
syslog_libc_log(struct syslogger *this OVS_UNUSED, int pri, const char *msg)
{
    syslog(pri, "%s", msg);
}
