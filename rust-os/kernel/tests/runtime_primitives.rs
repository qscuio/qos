use qos_kernel::{
    kthread_count, kthread_create, kthread_reset, kthread_run_count, kthread_run_next, kthread_stop,
    kthread_wake, napi_complete, napi_init, napi_pending_count, napi_register, napi_reset,
    napi_run_count, napi_schedule, softirq_pending_mask, softirq_raise, softirq_register, softirq_reset,
    softirq_run, timer_active_count, timer_add, timer_cancel, timer_init, timer_pending_count, timer_reset,
    timer_tick,
};
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{Mutex, OnceLock};

fn test_guard() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(())).lock().expect("mutex poisoned")
}

static SOFTIRQ_HITS: AtomicU32 = AtomicU32::new(0);
static TIMER_HITS: AtomicU32 = AtomicU32::new(0);
static KT1_HITS: AtomicU32 = AtomicU32::new(0);
static KT2_HITS: AtomicU32 = AtomicU32::new(0);
static NAPI_CALLS: AtomicU32 = AtomicU32::new(0);
static NAPI_MODE: AtomicU32 = AtomicU32::new(0);

fn softirq_cb(ctx: usize) {
    SOFTIRQ_HITS.fetch_add(ctx as u32, Ordering::SeqCst);
}

fn timer_cb(_ctx: usize) {
    TIMER_HITS.fetch_add(1, Ordering::SeqCst);
}

fn kthread_cb(arg: usize) {
    if arg == 1 {
        KT1_HITS.fetch_add(1, Ordering::SeqCst);
    } else if arg == 2 {
        KT2_HITS.fetch_add(1, Ordering::SeqCst);
    }
}

fn napi_poll(_ctx: usize, budget: u32) -> u32 {
    NAPI_CALLS.fetch_add(1, Ordering::SeqCst);
    let mode = NAPI_MODE.load(Ordering::SeqCst);
    if mode >= budget {
        NAPI_MODE.store(budget.saturating_sub(1), Ordering::SeqCst);
        budget
    } else {
        mode
    }
}

#[test]
fn softirq_register_raise_and_run() {
    let _guard = test_guard();
    SOFTIRQ_HITS.store(0, Ordering::SeqCst);

    softirq_reset();
    assert!(softirq_register(2, softirq_cb, 3));
    assert!(softirq_raise(2));
    assert_eq!(softirq_pending_mask(), 1 << 2);
    assert_eq!(softirq_run(), 1);
    assert_eq!(SOFTIRQ_HITS.load(Ordering::SeqCst), 3);
    assert_eq!(softirq_pending_mask(), 0);

    assert!(!softirq_raise(8));
}

#[test]
fn timer_one_shot_and_periodic_flow() {
    let _guard = test_guard();
    TIMER_HITS.store(0, Ordering::SeqCst);

    softirq_reset();
    timer_reset();
    timer_init();

    let t1 = timer_add(3, 0, timer_cb, 0).expect("one shot timer");
    assert_eq!(timer_active_count(), 1);

    timer_tick(2);
    assert_eq!(timer_pending_count(), 0);
    assert_eq!(TIMER_HITS.load(Ordering::SeqCst), 0);

    timer_tick(1);
    assert_eq!(timer_pending_count(), 1);
    assert!(softirq_pending_mask() & 1 != 0);
    assert!(softirq_run() >= 1);
    assert_eq!(TIMER_HITS.load(Ordering::SeqCst), 1);
    assert_eq!(timer_active_count(), 0);

    let t2 = timer_add(2, 2, timer_cb, 0).expect("periodic timer");
    timer_tick(2);
    assert!(softirq_run() >= 1);
    assert_eq!(TIMER_HITS.load(Ordering::SeqCst), 2);
    assert_eq!(timer_active_count(), 1);

    timer_tick(2);
    assert!(softirq_run() >= 1);
    assert_eq!(TIMER_HITS.load(Ordering::SeqCst), 3);

    assert!(timer_cancel(t2));
    assert_eq!(timer_active_count(), 0);
    assert!(!timer_cancel(t1));
}

#[test]
fn kthread_create_run_stop_and_wake() {
    let _guard = test_guard();
    KT1_HITS.store(0, Ordering::SeqCst);
    KT2_HITS.store(0, Ordering::SeqCst);

    kthread_reset();
    let tid1 = kthread_create(kthread_cb, 1).expect("tid1");
    let tid2 = kthread_create(kthread_cb, 2).expect("tid2");
    assert_ne!(tid1, tid2);
    assert_eq!(kthread_count(), 2);

    assert_eq!(kthread_run_next(), tid1);
    assert_eq!(kthread_run_next(), tid2);
    assert_eq!(KT1_HITS.load(Ordering::SeqCst), 1);
    assert_eq!(KT2_HITS.load(Ordering::SeqCst), 1);

    assert!(kthread_stop(tid1));
    assert_eq!(kthread_run_next(), tid2);
    assert_eq!(KT2_HITS.load(Ordering::SeqCst), 2);

    assert!(kthread_wake(tid1));
    assert_eq!(kthread_run_next(), tid1);
    assert_eq!(KT1_HITS.load(Ordering::SeqCst), 2);
    assert_eq!(kthread_run_count(tid1), 2);
    assert_eq!(kthread_run_count(tid2), 2);
    assert!(!kthread_stop(0));
}

#[test]
fn napi_schedule_poll_reschedule_and_complete() {
    let _guard = test_guard();
    NAPI_CALLS.store(0, Ordering::SeqCst);
    NAPI_MODE.store(0, Ordering::SeqCst);

    softirq_reset();
    napi_reset();
    napi_init();

    let napi_id = napi_register(4, napi_poll, 0).expect("napi id");

    NAPI_MODE.store(2, Ordering::SeqCst);
    assert!(napi_schedule(napi_id));
    assert_eq!(napi_pending_count(), 1);
    assert!(softirq_pending_mask() & (1 << 1) != 0);
    assert!(softirq_run() >= 1);
    assert_eq!(NAPI_CALLS.load(Ordering::SeqCst), 1);
    assert_eq!(napi_run_count(napi_id), 1);
    assert_eq!(napi_pending_count(), 0);

    NAPI_MODE.store(4, Ordering::SeqCst);
    assert!(napi_schedule(napi_id));
    assert!(softirq_run() >= 1);
    assert_eq!(NAPI_CALLS.load(Ordering::SeqCst), 3);
    assert_eq!(napi_run_count(napi_id), 3);
    assert_eq!(napi_pending_count(), 0);
    assert!(napi_complete(napi_id));
    assert!(!napi_schedule(0));
}
