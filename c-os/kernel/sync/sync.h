#ifndef QOS_SYNC_H
#define QOS_SYNC_H

#include <stdint.h>

typedef struct {
    volatile uint32_t locked;
} qos_spinlock_t;

typedef struct {
    uint32_t locked;
    uint32_t waiters;
} qos_mutex_t;

typedef struct {
    int32_t value;
    uint32_t waiters;
} qos_semaphore_t;

int qos_spin_init(qos_spinlock_t *lock);
int qos_spin_lock(qos_spinlock_t *lock);
int qos_spin_trylock(qos_spinlock_t *lock);
int qos_spin_unlock(qos_spinlock_t *lock);
int qos_spin_is_locked(const qos_spinlock_t *lock);

int qos_mutex_init(qos_mutex_t *mutex);
int qos_mutex_lock(qos_mutex_t *mutex);
int qos_mutex_trylock(qos_mutex_t *mutex);
int qos_mutex_unlock(qos_mutex_t *mutex);
uint32_t qos_mutex_waiters(const qos_mutex_t *mutex);
int qos_mutex_is_locked(const qos_mutex_t *mutex);

int qos_sem_init(qos_semaphore_t *sem, int32_t initial);
int qos_sem_wait(qos_semaphore_t *sem);
int qos_sem_post(qos_semaphore_t *sem);
int32_t qos_sem_value(const qos_semaphore_t *sem);
uint32_t qos_sem_waiters(const qos_semaphore_t *sem);

#endif
