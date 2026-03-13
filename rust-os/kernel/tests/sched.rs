use qos_kernel::{sched_add_task, sched_count, sched_current, sched_next, sched_remove_task, sched_reset};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

#[test]
fn sched_round_robin_order() {
    let _guard = test_guard();
    sched_reset();
    assert!(sched_add_task(1));
    assert!(sched_add_task(2));
    assert!(sched_add_task(3));
    assert_eq!(sched_count(), 3);
    assert_eq!(sched_next(), Some(1));
    assert_eq!(sched_current(), 1);
    assert_eq!(sched_next(), Some(2));
    assert_eq!(sched_current(), 2);
    assert_eq!(sched_next(), Some(3));
    assert_eq!(sched_current(), 3);
    assert_eq!(sched_next(), Some(1));
    assert_eq!(sched_current(), 1);
}

#[test]
fn sched_remove_and_duplicate_rules() {
    let _guard = test_guard();
    sched_reset();
    assert!(sched_add_task(7));
    assert!(!sched_add_task(7));
    assert!(sched_add_task(8));
    assert!(sched_remove_task(7));
    assert!(!sched_remove_task(7));
    assert_eq!(sched_next(), Some(8));
    assert_eq!(sched_current(), 8);
}
