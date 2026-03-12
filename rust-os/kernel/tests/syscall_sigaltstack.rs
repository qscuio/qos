use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, proc_create, proc_signal_get_altstack, syscall_dispatch, SYSCALL_NR_SIGALTSTACK,
};
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
fn syscall_sigaltstack_bridge() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(9, 0));

    assert_eq!(syscall_dispatch(SYSCALL_NR_SIGALTSTACK, 9, u64::MAX, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_SIGALTSTACK, 9, 0x900000, 0x4000, 1), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_SIGALTSTACK, 9, u64::MAX, 0, 0), 1);

    let alt = proc_signal_get_altstack(9).expect("altstack should exist");
    assert_eq!(alt.sp, 0x900000);
    assert_eq!(alt.size, 0x4000);
    assert_eq!(alt.flags, 1);

    assert_eq!(syscall_dispatch(SYSCALL_NR_SIGALTSTACK, 999, u64::MAX, 0, 0), -1);
}
