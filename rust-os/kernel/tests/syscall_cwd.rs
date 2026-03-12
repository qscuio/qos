use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, proc_create, syscall_dispatch, SYSCALL_NR_CHDIR, SYSCALL_NR_GETCWD,
    SYSCALL_NR_MKDIR,
};
use std::ffi::{CStr, CString};
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
fn syscall_chdir_getcwd_bridge() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(1, 0));

    let home = CString::new("/home").expect("cstring");
    let none = CString::new("/none").expect("cstring");
    let home_ptr = home.as_ptr() as u64;
    let none_ptr = none.as_ptr() as u64;

    assert_eq!(syscall_dispatch(SYSCALL_NR_MKDIR, home_ptr, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CHDIR, 1, home_ptr, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_GETCWD, 1, 0, 0, 0), 5);

    let mut out = [0u8; 32];
    let out_ptr = out.as_mut_ptr() as u64;
    assert_eq!(syscall_dispatch(SYSCALL_NR_GETCWD, 1, out_ptr, 32, 0), 5);
    let as_cstr = CStr::from_bytes_until_nul(&out).expect("nul terminated");
    assert_eq!(as_cstr.to_str().expect("utf8"), "/home");

    assert_eq!(syscall_dispatch(SYSCALL_NR_GETCWD, 1, out_ptr, 3, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CHDIR, 1, none_ptr, 0, 0), -1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_CHDIR, 999, home_ptr, 0, 0), -1);
}
