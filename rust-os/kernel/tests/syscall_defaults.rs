use qos_boot::boot_info::{BootInfo, MmapEntry, QOS_BOOT_MAGIC};
use qos_kernel::{
    drivers_register, kernel_entry, net_enqueue_packet, proc_create, sched_add_task, syscall_dispatch,
    syscall_register, vfs_create, QOS_INIT_ALL, SYSCALL_NR_QUERY_DRIVERS_COUNT,
    SYSCALL_NR_QUERY_INIT_STATE, SYSCALL_NR_QUERY_NET_QUEUE_LEN, SYSCALL_NR_QUERY_PMM_FREE,
    SYSCALL_NR_QUERY_PMM_TOTAL, SYSCALL_NR_QUERY_PROC_COUNT, SYSCALL_NR_QUERY_SCHED_COUNT,
    SYSCALL_NR_QUERY_SYSCALL_COUNT, SYSCALL_NR_QUERY_VFS_COUNT, SYSCALL_OP_CONST,
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
fn syscall_default_query_map_tracks_live_state() {
    let _guard = test_guard();
    let info = valid_boot_info();
    assert!(kernel_entry(&info).is_ok());

    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_INIT_STATE, 0, 0, 0, 0), QOS_INIT_ALL as i64);
    assert!(syscall_dispatch(SYSCALL_NR_QUERY_PMM_TOTAL, 0, 0, 0, 0) > 0);
    assert!(syscall_dispatch(SYSCALL_NR_QUERY_PMM_FREE, 0, 0, 0, 0) > 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_SCHED_COUNT, 0, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_VFS_COUNT, 0, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_NET_QUEUE_LEN, 0, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_DRIVERS_COUNT, 0, 0, 0, 0), 0);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_PROC_COUNT, 0, 0, 0, 0), 0);

    let base_syscalls = syscall_dispatch(SYSCALL_NR_QUERY_SYSCALL_COUNT, 0, 0, 0, 0);
    assert!(base_syscalls >= 9);

    assert!(vfs_create("/init"));
    assert!(drivers_register("uart"));
    assert!(proc_create(1, 0));
    assert!(sched_add_task(1));
    assert!(net_enqueue_packet(&[1, 2]));
    assert!(syscall_register(90, SYSCALL_OP_CONST, 42));

    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_VFS_COUNT, 0, 0, 0, 0), 1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_DRIVERS_COUNT, 0, 0, 0, 0), 1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_PROC_COUNT, 0, 0, 0, 0), 1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_SCHED_COUNT, 0, 0, 0, 0), 1);
    assert_eq!(syscall_dispatch(SYSCALL_NR_QUERY_NET_QUEUE_LEN, 0, 0, 0, 0), 1);
    assert_eq!(
        syscall_dispatch(SYSCALL_NR_QUERY_SYSCALL_COUNT, 0, 0, 0, 0),
        base_syscalls + 1
    );
}
