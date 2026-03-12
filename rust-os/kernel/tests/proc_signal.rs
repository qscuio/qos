use qos_kernel::{
    proc_create, proc_exec_signal_reset, proc_fork, proc_reset, proc_signal_get_handler,
    proc_signal_get_altstack, proc_signal_set_altstack,
    proc_signal_mask, proc_signal_next, proc_signal_pending, proc_signal_send,
    proc_signal_set_handler, proc_signal_set_mask, QOS_SIGKILL, QOS_SIGSTOP, QOS_SIGUSR1,
    QOS_SIGUSR2, QOS_SIG_DFL, QOS_SIG_IGN,
};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

fn bit(signum: u32) -> u32 {
    1u32 << signum
}

#[test]
fn proc_signal_mask_pending_and_delivery() {
    let _guard = test_guard();
    proc_reset();
    assert!(proc_create(10, 0));

    assert!(!proc_signal_set_handler(10, QOS_SIGKILL, 0x1234));
    assert!(proc_signal_set_handler(10, QOS_SIGUSR1, 0x1234));
    assert_eq!(proc_signal_get_handler(10, QOS_SIGUSR1), Some(0x1234));

    let mask = bit(QOS_SIGUSR1) | bit(QOS_SIGKILL) | bit(QOS_SIGSTOP);
    assert!(proc_signal_set_mask(10, mask));
    assert_eq!(proc_signal_mask(10), Some(bit(QOS_SIGUSR1)));

    assert!(proc_signal_send(10, QOS_SIGUSR1));
    assert!(proc_signal_send(10, QOS_SIGKILL));
    let pending = proc_signal_pending(10).expect("pending should exist");
    assert_ne!(pending & bit(QOS_SIGUSR1), 0);
    assert_ne!(pending & bit(QOS_SIGKILL), 0);

    assert_eq!(proc_signal_next(10), QOS_SIGKILL as i32);
    assert_eq!(proc_signal_next(10), 0);

    assert!(proc_signal_set_mask(10, 0));
    assert_eq!(proc_signal_next(10), QOS_SIGUSR1 as i32);
    assert_eq!(proc_signal_next(10), 0);
}

#[test]
fn proc_signal_fork_and_exec_semantics() {
    let _guard = test_guard();
    proc_reset();
    assert!(proc_create(1, 0));
    assert!(proc_signal_set_handler(1, QOS_SIGUSR1, QOS_SIG_IGN));
    assert!(proc_signal_set_handler(1, QOS_SIGUSR2, 0x2000));
    assert!(proc_signal_set_altstack(1, 0x800000, 0x4000, 1));
    assert!(proc_signal_set_mask(1, bit(QOS_SIGUSR2)));
    assert!(proc_signal_send(1, QOS_SIGUSR2));

    assert!(proc_fork(1, 2));

    assert_eq!(proc_signal_pending(2), Some(0));
    assert_eq!(proc_signal_mask(2), Some(bit(QOS_SIGUSR2)));
    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR1), Some(QOS_SIG_IGN));
    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR2), Some(0x2000));
    let child_alt = proc_signal_get_altstack(2).expect("child altstack should exist");
    assert_eq!(child_alt.sp, 0x800000);
    assert_eq!(child_alt.size, 0x4000);
    assert_eq!(child_alt.flags, 1);

    assert!(proc_exec_signal_reset(2));

    assert_eq!(proc_signal_pending(2), Some(0));
    assert_eq!(proc_signal_mask(2), Some(bit(QOS_SIGUSR2)));
    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR1), Some(QOS_SIG_IGN));
    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR2), Some(QOS_SIG_DFL));
}
