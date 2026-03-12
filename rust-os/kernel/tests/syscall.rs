use qos_kernel::{
    syscall_count, syscall_dispatch, syscall_register, syscall_reset, syscall_unregister,
    SYSCALL_OP_ADD, SYSCALL_OP_CONST,
};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

#[test]
fn syscall_register_dispatch_unregister() {
    let _guard = test_guard();
    syscall_reset();
    assert!(syscall_register(1, SYSCALL_OP_CONST, 42));
    assert!(syscall_register(2, SYSCALL_OP_ADD, 0));
    assert_eq!(syscall_count(), 2);
    assert_eq!(syscall_dispatch(1, 0, 0, 0, 0), 42);
    assert_eq!(syscall_dispatch(2, 7, 9, 0, 0), 16);
    assert!(syscall_unregister(1));
    assert_eq!(syscall_dispatch(1, 0, 0, 0, 0), -1);
    assert_eq!(syscall_count(), 1);
}

#[test]
fn syscall_rejects_invalid_inputs() {
    let _guard = test_guard();
    syscall_reset();
    assert!(syscall_register(7, SYSCALL_OP_CONST, 1));
    assert!(!syscall_register(7, SYSCALL_OP_CONST, 2));
    assert!(!syscall_register(9999, SYSCALL_OP_CONST, 1));
    assert!(!syscall_register(8, 99, 0));
}
