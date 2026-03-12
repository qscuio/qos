#include "sync.h"

int qos_spin_init(qos_spinlock_t *lock) {
    if (lock == 0) {
        return -1;
    }
    lock->locked = 0;
    return 0;
}

int qos_spin_trylock(qos_spinlock_t *lock) {
    if (lock == 0) {
        return -1;
    }
    return __sync_lock_test_and_set(&lock->locked, 1) == 0 ? 0 : -1;
}

int qos_spin_lock(qos_spinlock_t *lock) {
    if (lock == 0) {
        return -1;
    }
    while (qos_spin_trylock(lock) != 0) {
    }
    return 0;
}

int qos_spin_unlock(qos_spinlock_t *lock) {
    if (lock == 0 || lock->locked == 0) {
        return -1;
    }
    __sync_lock_release(&lock->locked);
    return 0;
}

int qos_spin_is_locked(const qos_spinlock_t *lock) {
    if (lock == 0) {
        return -1;
    }
    return lock->locked != 0 ? 1 : 0;
}

int qos_mutex_init(qos_mutex_t *mutex) {
    if (mutex == 0) {
        return -1;
    }
    mutex->locked = 0;
    mutex->waiters = 0;
    return 0;
}

int qos_mutex_lock(qos_mutex_t *mutex) {
    if (mutex == 0) {
        return -1;
    }
    if (mutex->locked == 0) {
        mutex->locked = 1;
        return 0;
    }
    mutex->waiters++;
    return -1;
}

int qos_mutex_trylock(qos_mutex_t *mutex) {
    if (mutex == 0) {
        return -1;
    }
    if (mutex->locked == 0) {
        mutex->locked = 1;
        return 0;
    }
    return -1;
}

int qos_mutex_unlock(qos_mutex_t *mutex) {
    if (mutex == 0 || mutex->locked == 0) {
        return -1;
    }
    if (mutex->waiters > 0) {
        mutex->waiters--;
    }
    mutex->locked = 0;
    return 0;
}

uint32_t qos_mutex_waiters(const qos_mutex_t *mutex) {
    if (mutex == 0) {
        return 0;
    }
    return mutex->waiters;
}

int qos_mutex_is_locked(const qos_mutex_t *mutex) {
    if (mutex == 0) {
        return -1;
    }
    return mutex->locked != 0 ? 1 : 0;
}

int qos_sem_init(qos_semaphore_t *sem, int32_t initial) {
    if (sem == 0 || initial < 0) {
        return -1;
    }
    sem->value = initial;
    sem->waiters = 0;
    return 0;
}

int qos_sem_wait(qos_semaphore_t *sem) {
    if (sem == 0) {
        return -1;
    }
    if (sem->value > 0) {
        sem->value--;
        return 0;
    }
    sem->waiters++;
    return -1;
}

int qos_sem_post(qos_semaphore_t *sem) {
    if (sem == 0) {
        return -1;
    }
    if (sem->waiters > 0) {
        sem->waiters--;
        return 1;
    }
    sem->value++;
    return 0;
}

int32_t qos_sem_value(const qos_semaphore_t *sem) {
    if (sem == 0) {
        return -1;
    }
    return sem->value;
}

uint32_t qos_sem_waiters(const qos_semaphore_t *sem) {
    if (sem == 0) {
        return 0;
    }
    return sem->waiters;
}
