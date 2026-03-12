use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, proc_alive, proc_create, proc_parent, proc_signal_get_altstack,
    proc_exec_image_get,
    proc_signal_get_handler, proc_signal_mask, proc_signal_pending, proc_signal_send,
    proc_signal_set_altstack, proc_signal_set_handler, proc_signal_set_mask, syscall_dispatch,
    QOS_SIGUSR1, QOS_SIGUSR2, QOS_SIG_DFL, QOS_SIG_IGN, SYSCALL_NR_CLOSE, SYSCALL_NR_EXEC, SYSCALL_NR_FORK,
    SYSCALL_NR_MKDIR, SYSCALL_NR_OPEN, SYSCALL_NR_WRITE,
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

fn bit(signum: u32) -> u32 {
    1u32 << signum
}

fn elf64_image() -> [u8; 256] {
    let mut image = [0u8; 256];
    image[0] = 0x7F;
    image[1] = b'E';
    image[2] = b'L';
    image[3] = b'F';
    image[4] = 2;
    image[5] = 1;
    image[6] = 1;

    image[16..18].copy_from_slice(&(2u16).to_le_bytes());
    image[18..20].copy_from_slice(&(0x3Eu16).to_le_bytes());
    image[20..24].copy_from_slice(&(1u32).to_le_bytes());
    image[24..32].copy_from_slice(&(0x401000u64).to_le_bytes());
    image[32..40].copy_from_slice(&(64u64).to_le_bytes());
    image[52..54].copy_from_slice(&(64u16).to_le_bytes());
    image[54..56].copy_from_slice(&(56u16).to_le_bytes());
    image[56..58].copy_from_slice(&(1u16).to_le_bytes());

    image[64..68].copy_from_slice(&(1u32).to_le_bytes());
    image[68..72].copy_from_slice(&(5u32).to_le_bytes());
    image[72..80].copy_from_slice(&(0u64).to_le_bytes());
    image[80..88].copy_from_slice(&(0x400000u64).to_le_bytes());
    image[88..96].copy_from_slice(&(0u64).to_le_bytes());
    image[96..104].copy_from_slice(&(128u64).to_le_bytes());
    image[104..112].copy_from_slice(&(128u64).to_le_bytes());
    image[112..120].copy_from_slice(&(0x1000u64).to_le_bytes());
    image
}

fn write_file_via_syscalls(path: &CString, data: &[u8]) {
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_MKDIR, path.as_ptr() as u64, 0, 0, 0),
        0
    );
    let fd = syscall_dispatch(SYSCALL_NR_OPEN, path.as_ptr() as u64, 0, 0, 0);
    assert!(fd >= 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_WRITE, fd as u64, data.as_ptr() as u64, data.len() as u64, 0),
        data.len() as i64
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0), 0);
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

#[test]
fn syscall_exec_loads_real_elf_image_from_path() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    assert!(proc_create(1, 0));
    assert!(proc_create(3, 1));

    let elf_path = CString::new("/tmp/elfcmd").expect("valid cstring");
    let elf = elf64_image();
    write_file_via_syscalls(&elf_path, &elf);
    assert_eq!(syscall_dispatch(SYSCALL_NR_EXEC, 3, elf_path.as_ptr() as u64, 0, 0), 0);

    let img = proc_exec_image_get(3).expect("exec image should be present");
    assert_eq!(img.entry, 0x401000);
    assert_eq!(img.phoff, 64);
    assert_eq!(img.phentsize, 56);
    assert_eq!(img.phnum, 1);
    assert_eq!(img.load_count, 1);
    assert!(!img.has_interp);

    let bad_path = CString::new("/tmp/notelf").expect("valid cstring");
    write_file_via_syscalls(&bad_path, b"not-elf");
    assert_eq!(syscall_dispatch(SYSCALL_NR_EXEC, 3, bad_path.as_ptr() as u64, 0, 0), -1);
}
