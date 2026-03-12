#include <string.h>

#include "../init_state.h"
#include "../kernel.h"
#include "proc.h"

static uint32_t g_pids[QOS_PROC_MAX];
static uint32_t g_ppids[QOS_PROC_MAX];
static uint8_t g_used[QOS_PROC_MAX];
static uint8_t g_exited[QOS_PROC_MAX];
static uint8_t g_stopped[QOS_PROC_MAX];
static int32_t g_exit_code[QOS_PROC_MAX];
static char g_cwds[QOS_PROC_MAX][QOS_PROC_CWD_MAX];
static uint8_t g_cwd_len[QOS_PROC_MAX];
static uint64_t g_sig_handlers[QOS_PROC_MAX][QOS_SIGNAL_MAX];
static uint32_t g_sig_pending[QOS_PROC_MAX];
static uint32_t g_sig_blocked[QOS_PROC_MAX];
static uint64_t g_sig_altstack_sp[QOS_PROC_MAX];
static uint64_t g_sig_altstack_size[QOS_PROC_MAX];
static uint32_t g_sig_altstack_flags[QOS_PROC_MAX];
static uint32_t g_count = 0;

static int signum_valid(uint32_t signum) {
    return signum > 0 && signum < QOS_SIGNAL_MAX;
}

static uint32_t signum_bit(uint32_t signum) {
    return 1u << signum;
}

static uint32_t unmaskable_bits(void) {
    return signum_bit(QOS_SIGKILL) | signum_bit(QOS_SIGSTOP);
}

int qos_proc_signal_apply_default(uint32_t pid, uint32_t signum);

static int cwd_set_idx(uint32_t idx, const char *path) {
    size_t len;
    if (path == 0 || path[0] != '/') {
        return -1;
    }
    len = strlen(path);
    if (len == 0 || len >= QOS_PROC_CWD_MAX) {
        return -1;
    }
    memcpy(g_cwds[idx], path, len + 1);
    g_cwd_len[idx] = (uint8_t)len;
    return 0;
}

static void init_signal_state(uint32_t idx) {
    memset(g_sig_handlers[idx], 0, sizeof(g_sig_handlers[idx]));
    g_sig_pending[idx] = 0;
    g_sig_blocked[idx] = 0;
    g_sig_altstack_sp[idx] = 0;
    g_sig_altstack_size[idx] = 0;
    g_sig_altstack_flags[idx] = 0;
}

static int find_pid(uint32_t pid) {
    uint32_t i = 0;
    while (i < QOS_PROC_MAX) {
        if (g_used[i] != 0 && g_pids[i] == pid) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

static int free_slot(void) {
    uint32_t i = 0;
    while (i < QOS_PROC_MAX) {
        if (g_used[i] == 0) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

void qos_proc_reset(void) {
    memset(g_pids, 0, sizeof(g_pids));
    memset(g_ppids, 0, sizeof(g_ppids));
    memset(g_used, 0, sizeof(g_used));
    memset(g_exited, 0, sizeof(g_exited));
    memset(g_stopped, 0, sizeof(g_stopped));
    memset(g_exit_code, 0, sizeof(g_exit_code));
    memset(g_cwds, 0, sizeof(g_cwds));
    memset(g_cwd_len, 0, sizeof(g_cwd_len));
    memset(g_sig_handlers, 0, sizeof(g_sig_handlers));
    memset(g_sig_pending, 0, sizeof(g_sig_pending));
    memset(g_sig_blocked, 0, sizeof(g_sig_blocked));
    memset(g_sig_altstack_sp, 0, sizeof(g_sig_altstack_sp));
    memset(g_sig_altstack_size, 0, sizeof(g_sig_altstack_size));
    memset(g_sig_altstack_flags, 0, sizeof(g_sig_altstack_flags));
    g_count = 0;
}

int qos_proc_create(uint32_t pid, uint32_t ppid) {
    int slot;

    if (pid == 0) {
        return -1;
    }
    if (find_pid(pid) >= 0) {
        return -1;
    }

    slot = free_slot();
    if (slot < 0) {
        return -1;
    }

    g_used[(uint32_t)slot] = 1;
    g_exited[(uint32_t)slot] = 0;
    g_stopped[(uint32_t)slot] = 0;
    g_exit_code[(uint32_t)slot] = 0;
    g_pids[(uint32_t)slot] = pid;
    g_ppids[(uint32_t)slot] = ppid;
    if (cwd_set_idx((uint32_t)slot, "/") != 0) {
        return -1;
    }
    init_signal_state((uint32_t)slot);
    g_count++;
    return 0;
}

int qos_proc_fork(uint32_t parent_pid, uint32_t child_pid) {
    int parent_idx;
    int child_idx;
    uint32_t s;

    if (child_pid == 0 || find_pid(child_pid) >= 0) {
        return -1;
    }

    parent_idx = find_pid(parent_pid);
    if (parent_idx < 0) {
        return -1;
    }

    child_idx = free_slot();
    if (child_idx < 0) {
        return -1;
    }

    g_used[(uint32_t)child_idx] = 1;
    g_exited[(uint32_t)child_idx] = 0;
    g_stopped[(uint32_t)child_idx] = 0;
    g_exit_code[(uint32_t)child_idx] = 0;
    g_pids[(uint32_t)child_idx] = child_pid;
    g_ppids[(uint32_t)child_idx] = parent_pid;
    memcpy(g_cwds[(uint32_t)child_idx], g_cwds[(uint32_t)parent_idx], QOS_PROC_CWD_MAX);
    g_cwd_len[(uint32_t)child_idx] = g_cwd_len[(uint32_t)parent_idx];
    for (s = 0; s < QOS_SIGNAL_MAX; s++) {
        g_sig_handlers[(uint32_t)child_idx][s] = g_sig_handlers[(uint32_t)parent_idx][s];
    }
    g_sig_blocked[(uint32_t)child_idx] = g_sig_blocked[(uint32_t)parent_idx];
    g_sig_pending[(uint32_t)child_idx] = 0;
    g_sig_altstack_sp[(uint32_t)child_idx] = g_sig_altstack_sp[(uint32_t)parent_idx];
    g_sig_altstack_size[(uint32_t)child_idx] = g_sig_altstack_size[(uint32_t)parent_idx];
    g_sig_altstack_flags[(uint32_t)child_idx] = g_sig_altstack_flags[(uint32_t)parent_idx];
    g_count++;
    return 0;
}

int qos_proc_remove(uint32_t pid) {
    int idx = find_pid(pid);
    if (idx < 0) {
        return -1;
    }

    g_used[(uint32_t)idx] = 0;
    g_exited[(uint32_t)idx] = 0;
    g_stopped[(uint32_t)idx] = 0;
    g_exit_code[(uint32_t)idx] = 0;
    g_pids[(uint32_t)idx] = 0;
    g_ppids[(uint32_t)idx] = 0;
    g_cwds[(uint32_t)idx][0] = '\0';
    g_cwd_len[(uint32_t)idx] = 0;
    init_signal_state((uint32_t)idx);
    g_count--;
    return 0;
}

int32_t qos_proc_parent(uint32_t pid) {
    int idx = find_pid(pid);
    if (idx < 0) {
        return -1;
    }
    return (int32_t)g_ppids[(uint32_t)idx];
}

int qos_proc_alive(uint32_t pid) {
    int idx = find_pid(pid);
    if (idx < 0) {
        return 0;
    }
    return g_exited[(uint32_t)idx] == 0 ? 1 : 0;
}

uint32_t qos_proc_count(void) {
    return g_count;
}

int qos_proc_exit(uint32_t pid, int32_t code) {
    int idx = find_pid(pid);
    if (idx < 0 || g_exited[(uint32_t)idx] != 0) {
        return -1;
    }
    g_exited[(uint32_t)idx] = 1;
    g_stopped[(uint32_t)idx] = 0;
    g_exit_code[(uint32_t)idx] = code;
    return 0;
}

int32_t qos_proc_wait(uint32_t parent_pid, int32_t pid, int32_t *out_status, uint32_t options) {
    int parent_idx = find_pid(parent_pid);
    int match_idx = -1;
    uint32_t i;

    if (parent_idx < 0) {
        return -1;
    }

    if (pid == -1) {
        for (i = 0; i < QOS_PROC_MAX; i++) {
            if (g_used[i] != 0 && g_ppids[i] == parent_pid && g_exited[i] != 0) {
                match_idx = (int)i;
                break;
            }
        }
    } else {
        match_idx = find_pid((uint32_t)pid);
        if (match_idx < 0) {
            return (options & QOS_WAIT_WNOHANG) != 0 ? 0 : -1;
        }
        if (g_ppids[(uint32_t)match_idx] != parent_pid) {
            return -1;
        }
        if (g_exited[(uint32_t)match_idx] == 0) {
            return (options & QOS_WAIT_WNOHANG) != 0 ? 0 : -1;
        }
    }

    if (match_idx < 0) {
        return (options & QOS_WAIT_WNOHANG) != 0 ? 0 : -1;
    }

    if (out_status != 0) {
        *out_status = g_exit_code[(uint32_t)match_idx];
    }

    pid = (int32_t)g_pids[(uint32_t)match_idx];
    (void)qos_proc_remove((uint32_t)pid);
    return pid;
}

int qos_proc_cwd_set(uint32_t pid, const char *path) {
    int idx = find_pid(pid);
    if (idx < 0) {
        return -1;
    }
    return cwd_set_idx((uint32_t)idx, path);
}

int32_t qos_proc_cwd_get(uint32_t pid, char *out, uint32_t out_len) {
    int idx = find_pid(pid);
    uint32_t len;
    if (idx < 0 || out == 0 || out_len == 0) {
        return -1;
    }
    len = (uint32_t)g_cwd_len[(uint32_t)idx];
    if (len + 1 > out_len) {
        return -1;
    }
    memcpy(out, g_cwds[(uint32_t)idx], len + 1);
    return (int32_t)len;
}

int qos_proc_exec_signal_reset(uint32_t pid) {
    int idx = find_pid(pid);
    uint32_t s;

    if (idx < 0) {
        return -1;
    }

    for (s = 1; s < QOS_SIGNAL_MAX; s++) {
        if (g_sig_handlers[(uint32_t)idx][s] != QOS_SIG_IGN) {
            g_sig_handlers[(uint32_t)idx][s] = QOS_SIG_DFL;
        }
    }
    g_sig_pending[(uint32_t)idx] = 0;
    g_sig_blocked[(uint32_t)idx] &= ~unmaskable_bits();
    return 0;
}

int qos_proc_signal_set_handler(uint32_t pid, uint32_t signum, uint64_t handler) {
    int idx = find_pid(pid);
    if (idx < 0 || !signum_valid(signum)) {
        return -1;
    }
    if ((signum == QOS_SIGKILL || signum == QOS_SIGSTOP) && handler != QOS_SIG_DFL) {
        return -1;
    }

    g_sig_handlers[(uint32_t)idx][signum] = handler;
    return 0;
}

int qos_proc_signal_get_handler(uint32_t pid, uint32_t signum, uint64_t *out_handler) {
    int idx = find_pid(pid);
    if (idx < 0 || !signum_valid(signum) || out_handler == 0) {
        return -1;
    }
    *out_handler = g_sig_handlers[(uint32_t)idx][signum];
    return 0;
}

int qos_proc_signal_set_mask(uint32_t pid, uint32_t mask) {
    int idx = find_pid(pid);
    if (idx < 0) {
        return -1;
    }
    g_sig_blocked[(uint32_t)idx] = mask & ~unmaskable_bits();
    return 0;
}

int qos_proc_signal_mask(uint32_t pid, uint32_t *out_mask) {
    int idx = find_pid(pid);
    if (idx < 0 || out_mask == 0) {
        return -1;
    }
    *out_mask = g_sig_blocked[(uint32_t)idx];
    return 0;
}

int qos_proc_signal_pending(uint32_t pid, uint32_t *out_pending) {
    int idx = find_pid(pid);
    if (idx < 0 || out_pending == 0) {
        return -1;
    }
    *out_pending = g_sig_pending[(uint32_t)idx];
    return 0;
}

int qos_proc_signal_send(uint32_t pid, uint32_t signum) {
    int idx = find_pid(pid);
    if (idx < 0 || !signum_valid(signum)) {
        return -1;
    }

    g_sig_pending[(uint32_t)idx] |= signum_bit(signum);
    return 0;
}

int32_t qos_proc_signal_next(uint32_t pid) {
    int idx = find_pid(pid);
    uint32_t pending;
    uint32_t deliverable;
    uint32_t s;

    if (idx < 0) {
        return -1;
    }

    pending = g_sig_pending[(uint32_t)idx];
    if (pending == 0) {
        return 0;
    }

    deliverable = (pending & ~g_sig_blocked[(uint32_t)idx]) | (pending & unmaskable_bits());
    if (deliverable == 0) {
        return 0;
    }

    for (s = 1; s < QOS_SIGNAL_MAX; s++) {
        uint32_t bit = signum_bit(s);
        if ((deliverable & bit) != 0) {
            uint64_t handler = g_sig_handlers[(uint32_t)idx][s];
            g_sig_pending[(uint32_t)idx] &= ~bit;
            if (handler == QOS_SIG_IGN) {
                continue;
            }
            if (handler == QOS_SIG_DFL) {
                (void)qos_proc_signal_apply_default(pid, s);
            }
            return (int32_t)s;
        }
    }

    return 0;
}

int qos_proc_signal_apply_default(uint32_t pid, uint32_t signum) {
    int idx = find_pid(pid);
    if (idx < 0 || !signum_valid(signum)) {
        return -1;
    }

    if (signum == 17u) {
        return 0;
    }
    if (signum == 18u) {
        g_stopped[(uint32_t)idx] = 0;
        return 0;
    }
    if (signum == QOS_SIGSTOP || signum == 20u || signum == 21u || signum == 22u) {
        g_stopped[(uint32_t)idx] = 1;
        return 0;
    }

    g_stopped[(uint32_t)idx] = 0;
    g_exited[(uint32_t)idx] = 1;
    g_exit_code[(uint32_t)idx] = 128 + (int32_t)signum;
    return 0;
}

int qos_proc_signal_prepare_frame(uint32_t pid, uint32_t signum, qos_sigframe_t *out_frame) {
    int idx = find_pid(pid);
    uint64_t handler;

    if (idx < 0 || !signum_valid(signum) || out_frame == 0) {
        return -1;
    }

    handler = g_sig_handlers[(uint32_t)idx][signum];
    if (handler == QOS_SIG_DFL || handler == QOS_SIG_IGN) {
        return -1;
    }

    out_frame->pretcode = QOS_SIG_TRAMPOLINE_VA;
    out_frame->signum = (int32_t)signum;
    out_frame->saved_mask = g_sig_blocked[(uint32_t)idx];
    out_frame->saved_handler = handler;
    return 0;
}

int qos_proc_signal_sigreturn(uint32_t pid, const qos_sigframe_t *frame) {
    int idx = find_pid(pid);
    if (idx < 0 || frame == 0) {
        return -1;
    }

    g_sig_blocked[(uint32_t)idx] = frame->saved_mask & ~unmaskable_bits();
    return 0;
}

int qos_proc_signal_altstack_set(uint32_t pid, uint64_t sp, uint64_t size, uint32_t flags) {
    int idx = find_pid(pid);
    if (idx < 0) {
        return -1;
    }
    g_sig_altstack_sp[(uint32_t)idx] = sp;
    g_sig_altstack_size[(uint32_t)idx] = size;
    g_sig_altstack_flags[(uint32_t)idx] = flags;
    return 0;
}

int qos_proc_signal_altstack_get(uint32_t pid, qos_sigaltstack_t *out_altstack) {
    int idx = find_pid(pid);
    if (idx < 0 || out_altstack == 0) {
        return -1;
    }
    out_altstack->sp = g_sig_altstack_sp[(uint32_t)idx];
    out_altstack->size = g_sig_altstack_size[(uint32_t)idx];
    out_altstack->flags = g_sig_altstack_flags[(uint32_t)idx];
    out_altstack->_pad = 0;
    return 0;
}

int qos_proc_is_stopped(uint32_t pid) {
    int idx = find_pid(pid);
    if (idx < 0) {
        return 0;
    }
    return g_stopped[(uint32_t)idx] != 0 ? 1 : 0;
}

void proc_init(void) {
    qos_proc_reset();
    qos_kernel_state_mark(QOS_INIT_PROC);
}
