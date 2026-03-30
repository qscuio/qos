#ifndef __HAVE_FATAL_SIGNAL_H__
#define __HAVE_FATAL_SIGNAL_H__

#include <signal.h>
#include <stdbool.h>

/* Basic interface. */
void fatal_signal_init(void);
void fatal_signal_add_hook(void (*hook_cb)(void *aux),
                           void (*cancel_cb)(void *aux), void *aux,
                           bool run_at_exit);
void fatal_signal_fork(void);
void fatal_signal_run(void);
void fatal_signal_wait(void);
void fatal_ignore_sigpipe(void);
void fatal_signal_atexit_handler(void);

/* Convenience functions for unlinking files upon termination.
 *
 * These functions also unlink the files upon normal process termination via
 * exit(). */
void fatal_signal_add_file_to_unlink(const char *);
void fatal_signal_remove_file_to_unlink(const char *);
int fatal_signal_unlink_file_now(const char *);

/* Interface for other code that catches one of our signals and needs to pass
 * it through. */
void fatal_signal_handler(int sig_nr);
void fatal_signal_block(sigset_t *prev_mask);

#endif /* __HAVE_FATAL_SIGNAL_H__ */
