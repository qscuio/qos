use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, syscall_dispatch, SYSCALL_NR_CLOSE, SYSCALL_NR_MKDIR, SYSCALL_NR_OPEN,
    SYSCALL_NR_READ, SYSCALL_NR_STAT, SYSCALL_NR_UNLINK, SYSCALL_NR_WRITE,
};
use std::ffi::CString;
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
fn syscall_stat_mkdir_unlink_bridge() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let path = CString::new("/tmp").expect("cstring");
    let p = path.as_ptr() as u64;

    assert_eq!(syscall_dispatch(SYSCALL_NR_STAT, p, 0, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, p, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_STAT, p, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, p, 0, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_UNLINK, p, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_STAT, p, 0, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_UNLINK, p, 0, 0, 0), -1);
}

#[test]
fn syscall_vfs_path_rejects_null() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    assert_eq!(syscall_dispatch(SYSCALL_NR_STAT, 0, 0, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, 0, 0, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_UNLINK, 0, 0, 0, 0), -1);
}

#[test]
fn syscall_unlink_recreate_clears_file_contents() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let path = CString::new("/tmp").expect("cstring");
    let p = path.as_ptr() as u64;
    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, p, 0, 0, 0), 0);

    let fd = syscall_dispatch(SYSCALL_NR_OPEN, p, 0, 0, 0);
    assert!(fd >= 0);
    let payload = *b"abc";
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_WRITE,
            fd as u64,
            payload.as_ptr() as u64,
            payload.len() as u64,
            0
        ),
        3
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0), 0);

    assert_eq!(syscall_dispatch(SYSCALL_NR_UNLINK, p, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, p, 0, 0, 0), 0);

    let fd2 = syscall_dispatch(SYSCALL_NR_OPEN, p, 0, 0, 0);
    assert!(fd2 >= 0);
    let mut out = [0u8; 16];
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_READ,
            fd2 as u64,
            out.as_mut_ptr() as u64,
            out.len() as u64,
            0
        ),
        0
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd2 as u64, 0, 0, 0), 0);
}
