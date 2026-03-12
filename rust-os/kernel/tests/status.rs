use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    drivers_register, kernel_entry, kernel_status, net_enqueue_packet, proc_create, sched_add_task,
    syscall_register, vfs_create, SYSCALL_OP_CONST,
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
fn kernel_status_snapshot_tracks_subsystems() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());
    let baseline = kernel_status();
    assert!(baseline.syscall_count >= 9);
    assert!(vfs_create("/init"));
    assert!(drivers_register("uart"));
    assert!(proc_create(1, 0));
    assert!(sched_add_task(1));
    assert!(syscall_register(90, SYSCALL_OP_CONST, 42));
    assert!(net_enqueue_packet(&[7, 9]));

    let status = kernel_status();
    assert!(status.pmm_total_pages > 0);
    assert!(status.pmm_free_pages > 0);
    assert_eq!(status.vfs_count, 1);
    assert_eq!(status.drivers_count, 1);
    assert_eq!(status.proc_count, 1);
    assert_eq!(status.sched_count, 1);
    assert_eq!(status.syscall_count, baseline.syscall_count + 1);
    assert_eq!(status.net_queue_len, 1);
}
