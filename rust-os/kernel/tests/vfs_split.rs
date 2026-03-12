use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, proc_create, syscall_dispatch, vfs_fs_kind, vfs_is_read_only, QOS_VFS_FS_EXT2,
    QOS_VFS_FS_INITRAMFS, QOS_VFS_FS_PROCFS, QOS_VFS_FS_TMPFS, SYSCALL_NR_CLOSE, SYSCALL_NR_MKDIR,
    SYSCALL_NR_OPEN, SYSCALL_NR_READ, SYSCALL_NR_UNLINK, SYSCALL_NR_WRITE,
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
fn vfs_classifies_mount_kinds_and_read_only() {
    let _guard = test_guard();
    assert_eq!(vfs_fs_kind("/init"), Some(QOS_VFS_FS_INITRAMFS));
    assert_eq!(vfs_fs_kind("/tmp/work"), Some(QOS_VFS_FS_TMPFS));
    assert_eq!(vfs_fs_kind("/proc/meminfo"), Some(QOS_VFS_FS_PROCFS));
    assert_eq!(vfs_fs_kind("/data/file"), Some(QOS_VFS_FS_EXT2));

    assert_eq!(vfs_is_read_only("/init"), Some(true));
    assert_eq!(vfs_is_read_only("/proc/meminfo"), Some(true));
    assert_eq!(vfs_is_read_only("/tmp/work"), Some(false));
    assert_eq!(vfs_is_read_only("/data/file"), Some(false));
    assert_eq!(vfs_is_read_only("tmp"), None);
}

#[test]
fn syscall_procfs_split_behavior() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(7, 0));

    let meminfo = CString::new("/proc/meminfo").expect("cstring");
    let p_meminfo = meminfo.as_ptr() as u64;

    let fd = syscall_dispatch(SYSCALL_NR_OPEN, p_meminfo, 0, 0, 0);
    assert!(fd >= 0);
    let mut out = [0u8; 256];
    let n = syscall_dispatch(
        SYSCALL_NR_READ,
        fd as u64,
        out.as_mut_ptr() as u64,
        out.len() as u64,
        0,
    );
    assert!(n > 0);
    let text = core::str::from_utf8(&out[..n as usize]).expect("utf8");
    assert!(text.contains("MemTotal:"));
    assert!(text.contains("MemFree:"));
    assert!(text.contains("ProcCount:"));

    let payload = *b"x";
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_WRITE, fd as u64, payload.as_ptr() as u64, 1, 0),
        -1
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0), 0);

    let status7 = CString::new("/proc/7/status").expect("cstring");
    let p_status7 = status7.as_ptr() as u64;
    let fd_status = syscall_dispatch(SYSCALL_NR_OPEN, p_status7, 0, 0, 0);
    assert!(fd_status >= 0);
    let mut out_status = [0u8; 256];
    let m = syscall_dispatch(
        SYSCALL_NR_READ,
        fd_status as u64,
        out_status.as_mut_ptr() as u64,
        out_status.len() as u64,
        0,
    );
    assert!(m > 0);
    let status_text = core::str::from_utf8(&out_status[..m as usize]).expect("utf8");
    assert!(status_text.contains("Pid:\t7"));
    assert!(status_text.contains("State:\tRunning"));
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd_status as u64, 0, 0, 0), 0);

    let missing = CString::new("/proc/999/status").expect("cstring");
    assert_eq!(syscall_dispatch(SYSCALL_NR_OPEN, missing.as_ptr() as u64, 0, 0, 0), -1);

    let proc_new = CString::new("/proc/new").expect("cstring");
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_MKDIR, proc_new.as_ptr() as u64, 0, 0, 0),
        -1
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_UNLINK, p_meminfo, 0, 0, 0), -1);

    let data_path = CString::new("/data/test").expect("cstring");
    let p_data = data_path.as_ptr() as u64;
    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, p_data, 0, 0, 0), 0);
    let fd_data = syscall_dispatch(SYSCALL_NR_OPEN, p_data, 0, 0, 0);
    assert!(fd_data >= 0);
    let data = *b"ok";
    assert_eq!(
        syscall_dispatch(
            SYSCALL_NR_WRITE,
            fd_data as u64,
            data.as_ptr() as u64,
            data.len() as u64,
            0
        ),
        2
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd_data as u64, 0, 0, 0), 0);
}
