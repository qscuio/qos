use qos_kernel::{vfs_count, vfs_create, vfs_exists, vfs_remove, vfs_reset};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

#[test]
fn vfs_create_exists_remove_flow() {
    let _guard = test_guard();
    vfs_reset();
    assert!(vfs_create("/init"));
    assert!(vfs_create("/etc/config"));
    assert!(vfs_exists("/init"));
    assert!(vfs_exists("/etc/config"));
    assert_eq!(vfs_count(), 2);
    assert!(vfs_remove("/init"));
    assert!(!vfs_exists("/init"));
    assert_eq!(vfs_count(), 1);
}

#[test]
fn vfs_rejects_duplicate_and_invalid_paths() {
    let _guard = test_guard();
    vfs_reset();
    assert!(vfs_create("/tmp"));
    assert!(!vfs_create("/tmp"));
    assert!(!vfs_create("tmp"));
    assert!(!vfs_create(""));
    assert!(!vfs_remove("/missing"));
}
