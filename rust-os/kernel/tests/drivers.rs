use qos_kernel::{
    drivers_count, drivers_get_nic, drivers_loaded, drivers_nic_advance_tx, drivers_nic_consume_rx,
    drivers_register, drivers_register_nic, drivers_reset,
};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

#[test]
fn driver_register_and_lookup() {
    let _guard = test_guard();
    drivers_reset();
    assert!(drivers_register("uart"));
    assert!(drivers_register("e1000"));
    assert!(drivers_loaded("uart"));
    assert!(drivers_loaded("e1000"));
    assert!(!drivers_loaded("virtio-net"));
    assert_eq!(drivers_count(), 2);
}

#[test]
fn driver_rejects_duplicate_and_invalid_names() {
    let _guard = test_guard();
    drivers_reset();
    assert!(drivers_register("uart"));
    assert!(!drivers_register("uart"));
    assert!(!drivers_register(""));
}

#[test]
fn driver_nic_metadata_and_ring_bookkeeping() {
    let _guard = test_guard();
    drivers_reset();

    assert!(drivers_register_nic("e1000", 0xFEE00000, 11, 16, 16));
    assert!(drivers_register_nic("virtio-net", 0x0A000000, 1, 16, 16));
    assert!(!drivers_register_nic("e1000", 0xFEE00000, 11, 16, 16));

    let nic = drivers_get_nic("e1000").expect("nic");
    assert_eq!(nic.mmio_base, 0xFEE00000);
    assert_eq!(nic.irq, 11);
    assert_eq!(nic.tx_desc_count, 16);
    assert_eq!(nic.rx_desc_count, 16);
    assert_eq!(nic.tx_head, 0);
    assert_eq!(nic.tx_tail, 0);
    assert_eq!(nic.rx_head, 0);
    assert_eq!(nic.rx_tail, 0);

    assert!(drivers_nic_advance_tx("e1000", 3));
    assert!(drivers_nic_consume_rx("e1000", 2));
    let nic = drivers_get_nic("e1000").expect("nic after update");
    assert_eq!(nic.tx_tail, 3);
    assert_eq!(nic.rx_head, 2);

    assert!(drivers_nic_advance_tx("e1000", 20));
    let nic = drivers_get_nic("e1000").expect("nic wrap");
    assert_eq!(nic.tx_tail, (3 + 20) % 16);
}
