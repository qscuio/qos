use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{init_state, kernel_entry, reset_init_state, KernelError, QOS_INIT_ALL};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

fn valid_boot_info() -> BootInfo {
    let mut info = BootInfo::default();
    info.magic = QOS_BOOT_MAGIC;
    info.mmap_entry_count = 1;
    info.mmap_entries[0] = MmapEntry {
        base: 0x100000,
        length: 0x200000,
        type_: 1,
        _pad: 0,
    };
    info.initramfs_addr = 0x400000;
    info.initramfs_size = 0x1000;
    info
}

#[test]
fn kernel_entry_initializes_all_subsystems() {
    let _guard = test_guard();
    let info = valid_boot_info();
    reset_init_state();
    assert!(kernel_entry(&info).is_ok());
    assert_eq!(init_state(), QOS_INIT_ALL);
}

#[test]
fn kernel_entry_rejects_invalid_boot_info() {
    let _guard = test_guard();
    let mut info = valid_boot_info();
    info.magic = 0;
    reset_init_state();
    assert_eq!(kernel_entry(&info), Err(KernelError::InvalidBootInfo));
    assert_eq!(init_state(), 0);
}
