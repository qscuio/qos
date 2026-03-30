#include "ovs-atomic.h"
#include "hash.h"
#include "ovs-thread.h"

#ifdef OVS_ATOMIC_LOCKED_IMPL
static struct ovs_mutex *
mutex_for_pointer(void *p)
{
    OVS_ALIGNED_STRUCT(CACHE_LINE_SIZE, aligned_mutex) {
        struct ovs_mutex mutex;
        char pad[PAD_SIZE(sizeof(struct ovs_mutex), CACHE_LINE_SIZE)];
    };

    static struct aligned_mutex atomic_mutexes[] = {
#define MUTEX_INIT { .mutex = OVS_MUTEX_INITIALIZER }
#define MUTEX_INIT4  MUTEX_INIT,  MUTEX_INIT,  MUTEX_INIT,  MUTEX_INIT
#define MUTEX_INIT16 MUTEX_INIT4, MUTEX_INIT4, MUTEX_INIT4, MUTEX_INIT4
        MUTEX_INIT16, MUTEX_INIT16,
    };
    BUILD_ASSERT_DECL(IS_POW2(ARRAY_SIZE(atomic_mutexes)));

    uint32_t hash = hash_pointer(p, 0);
    uint32_t indx = hash & (ARRAY_SIZE(atomic_mutexes) - 1);
    return &atomic_mutexes[indx].mutex;
}

void
atomic_lock__(void *p)
    OVS_ACQUIRES(mutex_for_pointer(p))
{
    ovs_mutex_lock(mutex_for_pointer(p));
}

void
atomic_unlock__(void *p)
    OVS_RELEASES(mutex_for_pointer(p))
{
    ovs_mutex_unlock(mutex_for_pointer(p));
}
#endif /* OVS_ATOMIC_LOCKED_IMPL */
