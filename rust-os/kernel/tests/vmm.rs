use qos_kernel::{vmm_flags, vmm_map, vmm_reset, vmm_translate, vmm_unmap, VM_EXEC, VM_READ, VM_WRITE};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

#[test]
fn vmm_map_translate_unmap_round_trip() {
    let _guard = test_guard();
    vmm_reset();
    assert!(vmm_map(0x4000, 0x200000, VM_READ | VM_WRITE).is_ok());
    assert_eq!(vmm_translate(0x4000), Some(0x200000));
    assert_eq!(vmm_translate(0x4123), Some(0x200123));
    assert_eq!(vmm_flags(0x4000), VM_READ | VM_WRITE);
    assert!(vmm_unmap(0x4000));
    assert_eq!(vmm_translate(0x4000), None);
}

#[test]
fn vmm_rejects_unaligned_addresses() {
    let _guard = test_guard();
    vmm_reset();
    assert!(vmm_map(0x4001, 0x200000, VM_READ).is_err());
    assert!(vmm_map(0x4000, 0x200001, VM_READ).is_err());
    assert!(!vmm_unmap(0x4001));
}

#[test]
fn vmm_remap_updates_translation() {
    let _guard = test_guard();
    vmm_reset();
    assert!(vmm_map(0x8000, 0x300000, VM_READ).is_ok());
    assert!(vmm_map(0x8000, 0x500000, VM_EXEC).is_ok());
    assert_eq!(vmm_translate(0x8000), Some(0x500000));
    assert_eq!(vmm_flags(0x8000), VM_EXEC);
}
