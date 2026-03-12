use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, proc_create, proc_signal_mask, proc_signal_set_mask, syscall_dispatch,
    QOS_SIGUSR1, QOS_SIGUSR2, SYSCALL_NR_SIGRETURN,
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

fn bit(signum: u32) -> u32 {
    1u32 << signum
}

#[test]
fn syscall_sigreturn_restores_mask() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(13, 0));
    assert!(proc_signal_set_mask(13, bit(QOS_SIGUSR2)));

    let restore = bit(QOS_SIGUSR1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_SIGRETURN, 13, restore as u64, 0, 0), 0);
    assert_eq!(proc_signal_mask(13), Some(restore));
}

#[test]
fn syscall_sigreturn_rejects_invalid_pid() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert_eq!(syscall_dispatch(SYSCALL_NR_SIGRETURN, 999, 0, 0, 0), -1);
}
