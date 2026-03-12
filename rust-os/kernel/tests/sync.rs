use qos_kernel::{
    mutex_init, mutex_is_locked, mutex_lock, mutex_trylock, mutex_unlock, mutex_waiters, sem_init,
    sem_post, sem_value, sem_wait, sem_waiters, spin_init, spin_is_locked, spin_lock, spin_trylock,
    spin_unlock, KernelMutex, KernelSemaphore, SpinLock,
};

#[test]
fn spinlock_flow() {
    let mut lock = SpinLock::default();
    spin_init(&mut lock);
    assert!(!spin_is_locked(&lock));
    assert!(spin_trylock(&mut lock));
    assert!(spin_is_locked(&lock));
    assert!(!spin_trylock(&mut lock));
    assert!(spin_unlock(&mut lock));
    assert!(!spin_is_locked(&lock));
    assert!(spin_lock(&mut lock));
    assert!(spin_is_locked(&lock));
}

#[test]
fn mutex_and_semaphore_flow() {
    let mut m = KernelMutex::default();
    mutex_init(&mut m);
    assert!(mutex_lock(&mut m));
    assert!(!mutex_trylock(&mut m));
    assert!(!mutex_lock(&mut m));
    assert_eq!(mutex_waiters(&m), 1);
    assert!(mutex_unlock(&mut m));
    assert_eq!(mutex_waiters(&m), 0);
    assert!(mutex_is_locked(&m));
    assert!(mutex_unlock(&mut m));
    assert!(!mutex_is_locked(&m));

    let mut s = KernelSemaphore::default();
    sem_init(&mut s, 1);
    assert!(sem_wait(&mut s));
    assert_eq!(sem_value(&s), 0);
    assert!(!sem_wait(&mut s));
    assert_eq!(sem_waiters(&s), 1);
    assert!(sem_post(&mut s));
    assert_eq!(sem_waiters(&s), 0);
    assert_eq!(sem_value(&s), 0);
    assert!(!sem_post(&mut s));
    assert_eq!(sem_value(&s), 1);
}
