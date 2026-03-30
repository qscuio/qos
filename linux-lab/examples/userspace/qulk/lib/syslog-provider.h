#ifndef __HAVE_SYSLOG_PROVIDER_H__
#define __HAVE_SYSLOG_PROVIDER_H__


/* Open vSwitch interface to syslog daemon's interface.
 *
 * 'syslogger' is the base class that provides abstraction. */
struct syslogger {
    const struct syslog_class *class;  /* Virtual functions for concrete
                                        * syslogger implementations. */
    const char *prefix;                /* Prefix that is enforced by concrete
                                        * syslogger implementation.  Used
                                        * in vlog/list-pattern function. */
};

/* Each concrete syslogger implementation must define it's own table with
 * following functions.  These functions must never call any other VLOG_
 * function to prevent deadlocks. */
struct syslog_class {
    /* openlog() function should be called before syslog() function.  It
     * should initialize all system resources needed to perform logging. */
    void (*openlog)(struct syslogger *this, int facility);

    /* syslog() function sends message 'msg' to syslog daemon. */
    void (*syslog)(struct syslogger *this, int pri, const char *msg);
};

static inline const char *
syslog_get_prefix(struct syslogger *this)
{
    return this->prefix;
}

#endif /* __HAVE_SYSLOG_PROVIDER_H__ */
