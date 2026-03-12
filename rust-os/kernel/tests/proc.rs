use qos_kernel::{proc_alive, proc_count, proc_create, proc_parent, proc_remove, proc_reset};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

#[test]
fn proc_create_lookup_remove_flow() {
    let _guard = test_guard();
    proc_reset();
    assert!(proc_create(1, 0));
    assert!(proc_create(2, 1));
    assert!(proc_create(3, 1));
    assert_eq!(proc_count(), 3);
    assert_eq!(proc_parent(2), Some(1));
    assert!(proc_alive(3));
    assert!(proc_remove(3));
    assert!(!proc_alive(3));
    assert_eq!(proc_count(), 2);
}

#[test]
fn proc_rejects_duplicates_and_invalid_pid() {
    let _guard = test_guard();
    proc_reset();
    assert!(!proc_create(0, 0));
    assert!(proc_create(7, 0));
    assert!(!proc_create(7, 0));
    assert!(!proc_remove(9));
}
