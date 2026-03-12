use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    pmm_alloc_page, pmm_free_page, pmm_free_pages, pmm_init_from_boot_info, pmm_reset,
    pmm_total_pages, KernelError,
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
        length: 0x40000, // 64 pages
        type_: 1,
        _pad: 0,
    };
    info.initramfs_addr = 0x120000;
    info.initramfs_size = 0x2000; // 2 pages reserved
    info
}

#[test]
fn pmm_init_counts_usable_and_reserved_pages() {
    let _guard = test_guard();
    let info = valid_boot_info();
    pmm_reset();
    assert!(pmm_init_from_boot_info(&info).is_ok());
    assert_eq!(pmm_total_pages(), 64);
    assert_eq!(pmm_free_pages(), 62);
}

#[test]
fn pmm_alloc_and_free_round_trip() {
    let _guard = test_guard();
    let info = valid_boot_info();
    pmm_reset();
    assert!(pmm_init_from_boot_info(&info).is_ok());

    let before = pmm_free_pages();
    let page = pmm_alloc_page().expect("expected a free page");
    assert_eq!(pmm_free_pages(), before - 1);
    assert!(pmm_free_page(page));
    assert_eq!(pmm_free_pages(), before);
}

#[test]
fn pmm_init_rejects_invalid_boot_info() {
    let _guard = test_guard();
    let mut info = valid_boot_info();
    info.magic = 0;
    pmm_reset();
    assert_eq!(pmm_init_from_boot_info(&info), Err(KernelError::InvalidBootInfo));
}
