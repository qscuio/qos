use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    kernel_entry, syscall_dispatch, SYSCALL_NR_CLOSE, SYSCALL_NR_DLCLOSE, SYSCALL_NR_DLOPEN,
    SYSCALL_NR_DLSYM, SYSCALL_NR_MKDIR, SYSCALL_NR_MODLOAD, SYSCALL_NR_MODRELOAD,
    SYSCALL_NR_MODUNLOAD, SYSCALL_NR_OPEN, SYSCALL_NR_WRITE,
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

fn elf64_image(e_type: u16) -> [u8; 256] {
    let mut image = [0u8; 256];
    image[0] = 0x7F;
    image[1] = b'E';
    image[2] = b'L';
    image[3] = b'F';
    image[4] = 2;
    image[5] = 1;
    image[6] = 1;

    image[16..18].copy_from_slice(&e_type.to_le_bytes());
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
fn syscall_shared_lib_and_module_hot_reload_flow() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let so_path = CString::new("/tmp/libplug.so").expect("valid cstring");
    let so_img = elf64_image(3);
    write_file_via_syscalls(&so_path, &so_img);

    let handle1 = syscall_dispatch(SYSCALL_NR_DLOPEN, so_path.as_ptr() as u64, 0, 0, 0);
    assert!(handle1 > 0);
    let handle2 = syscall_dispatch(SYSCALL_NR_DLOPEN, so_path.as_ptr() as u64, 0, 0, 0);
    assert_eq!(handle2, handle1);

    let sym_name = CString::new("plugin_init").expect("valid cstring");
    let sym_addr = syscall_dispatch(SYSCALL_NR_DLSYM, handle1 as u64, sym_name.as_ptr() as u64, 0, 0);
    assert!(sym_addr > 0);

    assert_eq!(syscall_dispatch(SYSCALL_NR_DLCLOSE, handle1 as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_DLCLOSE, handle1 as u64, 0, 0, 0), 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_DLSYM, handle1 as u64, sym_name.as_ptr() as u64, 0, 0),
        -1
    );

    let module_id = syscall_dispatch(SYSCALL_NR_MODLOAD, so_path.as_ptr() as u64, 0, 0, 0);
    assert!(module_id > 0);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_MODLOAD, so_path.as_ptr() as u64, 0, 0, 0),
        module_id
    );
    assert_eq!(syscall_dispatch(SYSCALL_NR_MODRELOAD, module_id as u64, 0, 0, 0), 2);
    assert_eq!(syscall_dispatch(SYSCALL_NR_MODRELOAD, module_id as u64, 0, 0, 0), 3);
    assert_eq!(syscall_dispatch(SYSCALL_NR_MODUNLOAD, module_id as u64, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_MODRELOAD, module_id as u64, 0, 0, 0), -1);
}

#[test]
fn syscall_rejects_non_shared_images_for_dlopen_and_modload() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    let exec_path = CString::new("/tmp/not_shared.elf").expect("valid cstring");
    let exec_img = elf64_image(2);
    write_file_via_syscalls(&exec_path, &exec_img);

    assert_eq!(
        syscall_dispatch(SYSCALL_NR_DLOPEN, exec_path.as_ptr() as u64, 0, 0, 0),
        -1
    );
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_MODLOAD, exec_path.as_ptr() as u64, 0, 0, 0),
        -1
    );
}
