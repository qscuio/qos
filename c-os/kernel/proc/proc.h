#ifndef QOS_PROC_H
#define QOS_PROC_H

#include <stdint.h>

#define QOS_PROC_MAX 256U
#define QOS_SIGNAL_MAX 32U
#define QOS_WAIT_WNOHANG 1u
#define QOS_PROC_CWD_MAX 128U
#define QOS_SIG_TRAMPOLINE_VA 0x00007FFF00000000ULL

enum {
    QOS_SIG_DFL = 0u,
    QOS_SIG_IGN = 1u,
    QOS_SIGKILL = 9u,
    QOS_SIGUSR1 = 10u,
    QOS_SIGUSR2 = 12u,
    QOS_SIGSTOP = 19u,
};

typedef struct {
    uint64_t sp;
    uint64_t size;
    uint32_t flags;
    uint32_t _pad;
} qos_sigaltstack_t;

typedef struct {
    uint64_t pretcode;
    int32_t signum;
    uint32_t saved_mask;
    uint64_t saved_handler;
} qos_sigframe_t;

void qos_proc_reset(void);
int qos_proc_create(uint32_t pid, uint32_t ppid);
int qos_proc_remove(uint32_t pid);
int32_t qos_proc_parent(uint32_t pid);
int qos_proc_alive(uint32_t pid);
uint32_t qos_proc_count(void);
int qos_proc_fork(uint32_t parent_pid, uint32_t child_pid);
int qos_proc_exit(uint32_t pid, int32_t code);
int32_t qos_proc_wait(uint32_t parent_pid, int32_t pid, int32_t *out_status, uint32_t options);
int qos_proc_exec_signal_reset(uint32_t pid);
int qos_proc_cwd_set(uint32_t pid, const char *path);
int32_t qos_proc_cwd_get(uint32_t pid, char *out, uint32_t out_len);

int qos_proc_signal_set_handler(uint32_t pid, uint32_t signum, uint64_t handler);
int qos_proc_signal_get_handler(uint32_t pid, uint32_t signum, uint64_t *out_handler);
int qos_proc_signal_set_mask(uint32_t pid, uint32_t mask);
int qos_proc_signal_mask(uint32_t pid, uint32_t *out_mask);
int qos_proc_signal_pending(uint32_t pid, uint32_t *out_pending);
int qos_proc_signal_send(uint32_t pid, uint32_t signum);
int32_t qos_proc_signal_next(uint32_t pid);
int qos_proc_signal_apply_default(uint32_t pid, uint32_t signum);
int qos_proc_signal_prepare_frame(uint32_t pid, uint32_t signum, qos_sigframe_t *out_frame);
int qos_proc_signal_sigreturn(uint32_t pid, const qos_sigframe_t *frame);
int qos_proc_signal_altstack_set(uint32_t pid, uint64_t sp, uint64_t size, uint32_t flags);
int qos_proc_signal_altstack_get(uint32_t pid, qos_sigaltstack_t *out_altstack);
int qos_proc_is_stopped(uint32_t pid);

void proc_init(void);

#endif
