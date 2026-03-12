use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, proc_create, syscall_dispatch, QOS_WAIT_WNOHANG, SYSCALL_NR_EXIT,
    SYSCALL_NR_GETPID, SYSCALL_NR_WAITPID,
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
fn syscall_exit_getpid_waitpid_bridge() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(1, 0));
    assert!(proc_create(2, 1));

    let mut status: i32 = -1;

    assert_eq!(syscall_dispatch(SYSCALL_NR_GETPID, 2, 0, 0, 0), 2);
    assert_eq!(syscall_dispatch(SYSCALL_NR_EXIT, 2, 5, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_GETPID, 2, 0, 0, 0), -1);

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_WAITPID, 1, 2, (&mut status as *mut i32) as u64, 0),
        2
    );
    assert_eq!(status, 5);
    assert_eq!(syscall_dispatch(SYSCALL_NR_WAITPID, 1, 2, 0, 0), -1);

    assert!(proc_create(3, 1));
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_WAITPID, 1, 3, 0, QOS_WAIT_WNOHANG as u64),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_EXIT, 3, 9, 0, 0), 0);
    let mut status2: i32 = -1;
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_WAITPID,
            1,
            u64::MAX,
            (&mut status2 as *mut i32) as u64,
            0
        ),
        3
    );
    assert_eq!(status2, 9);
}

#[test]
fn syscall_waitpid_rejects_invalid_parent() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert_eq!(syscall_dispatch(SYSCALL_NR_WAITPID, 999, u64::MAX, 0, 0), -1);
}
