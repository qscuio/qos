use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, proc_alive, proc_create, proc_parent, proc_signal_get_altstack,
    proc_signal_get_handler, proc_signal_mask, proc_signal_pending, proc_signal_send,
    proc_signal_set_altstack, proc_signal_set_handler, proc_signal_set_mask, syscall_dispatch,
    QOS_SIGUSR1, QOS_SIGUSR2, QOS_SIG_DFL, QOS_SIG_IGN, SYSCALL_NR_EXEC, SYSCALL_NR_FORK,
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
fn syscall_fork_exec_bridge_and_signal_semantics() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(1, 0));
    assert!(proc_signal_set_handler(1, QOS_SIGUSR1, QOS_SIG_IGN));
    assert!(proc_signal_set_handler(1, QOS_SIGUSR2, 0x2000));
    assert!(proc_signal_set_mask(1, bit(QOS_SIGUSR2)));
    assert!(proc_signal_set_altstack(1, 0x800000, 0x3000, 2));
    assert!(proc_signal_send(1, QOS_SIGUSR2));

    assert_eq!(syscall_dispatch(SYSCALL_NR_FORK, 1, 2, 0, 0), 2);
    assert!(proc_alive(2));
    assert_eq!(proc_parent(2), Some(1));

    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR1), Some(QOS_SIG_IGN));
    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR2), Some(0x2000));
    assert_eq!(proc_signal_mask(2), Some(bit(QOS_SIGUSR2)));
    assert_eq!(proc_signal_pending(2), Some(0));

    let child_alt = proc_signal_get_altstack(2).expect("child altstack should exist");
    assert_eq!(child_alt.sp, 0x800000);
    assert_eq!(child_alt.size, 0x3000);
    assert_eq!(child_alt.flags, 2);

    assert!(proc_signal_send(2, QOS_SIGUSR2));
    assert_eq!(syscall_dispatch(SYSCALL_NR_EXEC, 2, 0, 0, 0), 0);
    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR1), Some(QOS_SIG_IGN));
    assert_eq!(proc_signal_get_handler(2, QOS_SIGUSR2), Some(QOS_SIG_DFL));
    assert_eq!(proc_signal_pending(2), Some(0));
}

#[test]
fn syscall_fork_exec_reject_invalid_inputs() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(1, 0));
    assert_eq!(syscall_dispatch(SYSCALL_NR_FORK, 999, 2, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_FORK, 1, 0, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_EXEC, 999, 0, 0, 0), -1);
}
