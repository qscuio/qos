/*
 * Copyright (c) 2014, 2015, 2016 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OVS_RCU_H
#define OVS_RCU_H 1

/* Read-Copy-Update (RCU)
 * ======================
 *
 * Introduction
 * ------------
 *
 * Atomic pointer access makes it pretty easy to implement lock-free
 * algorithms.  There is one big problem, though: when a writer updates a
 * pointer to point to a new data structure, some thread might be reading the
 * old version, and there's no convenient way to free the old version when all
 * threads are done with the old version.
 *
 * The function ovsrcu_postpone() solves that problem.  The function pointer
 * passed in as its argument is called only after all threads are done with old
 * versions of data structures.  The function callback frees an old version of
 * data no longer in use.  This technique is called "read-copy-update", or RCU
 * for short.
 *
 *
 * Details
 * -------
 *
 * A "quiescent state" is a time at which a thread holds no pointers to memory
 * that is managed by RCU; that is, when the thread is known not to reference
 * memory that might be an old version of some object freed via RCU.  For
 * example, poll_block() includes a quiescent state.
 *
 * The following functions manage the recognition of quiescent states:
 *
 *     void ovsrcu_quiesce(void)
 *
 *         Recognizes a momentary quiescent state in the current thread.
 *
 *     void ovsrcu_quiesce_start(void)
 *     void ovsrcu_quiesce_end(void)
 *
 *         Brackets a time period during which the current thread is quiescent.
 *
 * A newly created thread is initially active, not quiescent. When a process
 * becomes multithreaded, the main thread becomes active, not quiescent.
 *
 * When a quiescient state has occurred in every thread, we say that a "grace
 * period" has occurred.  Following a grace period, all of the callbacks
 * postponed before the start of the grace period MAY be invoked.  OVS takes
 * care of this automatically through the RCU mechanism: while a process still
 * has only a single thread, it invokes the postponed callbacks directly from
 * ovsrcu_quiesce() and ovsrcu_quiesce_start(); after additional threads have
 * been created, it creates an extra helper thread to invoke callbacks.
 *
 * Please note that while a postponed function call is guaranteed to happen
 * after the next time all participating threads have quiesced at least once,
 * there is no quarantee that all postponed functions are called as early as
 * possible, or that the functions postponed by different threads would be
 * called in the order the registrations took place.  In particular, even if
 * two threads provably postpone a function each in a specific order, the
 * postponed functions may still be called in the opposite order, depending on
 * the timing of when the threads call ovsrcu_quiesce(), how many functions
 * they postpone, and when the ovs-rcu thread happens to grab the functions to
 * be called.
 *
 * All functions postponed by a single thread are guaranteed to execute in the
 * order they were postponed, however.
 *
 * Usage
 * -----
 *
 * Use OVSRCU_TYPE(TYPE) to declare a pointer to RCU-protected data, e.g. the
 * following declares an RCU-protected "struct flow *" named flowp:
 *
 *     OVSRCU_TYPE(struct flow *) flowp;
 *
 * Use ovsrcu_get(TYPE, VAR) to read an RCU-protected pointer, e.g. to read the
 * pointer variable declared above:
 *
 *     struct flow *flow = ovsrcu_get(struct flow *, &flowp);
 *
 * If the pointer variable is currently protected against change (because
 * the current thread holds a mutex that protects it), ovsrcu_get_protected()
 * may be used instead.  Only on the Alpha architecture is this likely to
 * generate different code, but it may be useful documentation.
 *
 * (With GNU C or Clang, you get a compiler error if TYPE is wrong; other
 * compilers will merrily carry along accepting the wrong type.)
 *
 * Use ovsrcu_set() to write an RCU-protected pointer and ovsrcu_postpone() to
 * free the previous data.  ovsrcu_set_hidden() can be used on RCU protected
 * data not visible to any readers yet, but will be made visible by a later
 * ovsrcu_set().   ovsrcu_init() can be used to initialize RCU pointers when
 * no readers are yet executing.  If more than one thread can write the
 * pointer, then some form of external synchronization, e.g. a mutex, is
 * needed to prevent writers from interfering with one another.  For example,
 * to write the pointer variable declared above while safely freeing the old
 * value:
 *
 *     static struct ovs_mutex mutex = OVS_MUTEX_INITIALIZER;
 *
 *     OVSRCU_TYPE(struct flow *) flowp;
 *
 *     void
 *     change_flow(struct flow *new_flow)
 *     {
 *         ovs_mutex_lock(&mutex);
 *         ovsrcu_postpone(free,
 *                         ovsrcu_get_protected(struct flow *, &flowp));
 *         ovsrcu_set(&flowp, new_flow);
 *         ovs_mutex_unlock(&mutex);
 *     }
 *
 * In some rare cases an object may not be addressable with a pointer, but only
 * through an array index (e.g. because it's provided by another library).  It
 * is still possible to have RCU semantics by using the ovsrcu_index type.
 *
 *     static struct ovs_mutex mutex = OVS_MUTEX_INITIALIZER;
 *
 *     ovsrcu_index port_id;
 *
 *     void tx()
 *     {
 *         int id = ovsrcu_index_get(&port_id);
 *         if (id == -1) {
 *             return;
 *         }
 *         port_tx(id);
 *     }
 *
 *     void delete()
 *     {
 *         int id;
 *
 *         ovs_mutex_lock(&mutex);
 *         id = ovsrcu_index_get_protected(&port_id);
 *         ovsrcu_index_set(&port_id, -1);
 *         ovs_mutex_unlock(&mutex);
 *
 *         ovsrcu_synchronize();
 *         port_delete(id);
 *     }
 *
 */

#include "compiler.h"
#include "ovs-atomic.h"

#if __GNUC__
#define OVSRCU_TYPE(TYPE) struct { ATOMIC(TYPE) p; }
#define OVSRCU_INITIALIZER(VALUE) { ATOMIC_VAR_INIT(VALUE) }
#define ovsrcu_get__(TYPE, VAR, ORDER)                                  \
    ({                                                                  \
        TYPE value__;                                                   \
        typeof(VAR) ovsrcu_var = (VAR);                                 \
                                                                        \
        atomic_read_explicit(CONST_CAST(ATOMIC(TYPE) *, &ovsrcu_var->p), \
                             &value__, ORDER);                          \
                                                                        \
        value__;                                                        \
    })
#define ovsrcu_get(TYPE, VAR) \
    ovsrcu_get__(TYPE, VAR, memory_order_consume)
#define ovsrcu_get_protected(TYPE, VAR) \
    ovsrcu_get__(TYPE, VAR, memory_order_relaxed)

/* 'VALUE' may be an atomic operation, which must be evaluated before
 * any of the body of the atomic_store_explicit.  Since the type of
 * 'VAR' is not fixed, we cannot use an inline function to get
 * function semantics for this. */
#define ovsrcu_set__(VAR, VALUE, ORDER)                                 \
    ({                                                                  \
        typeof(VAR) ovsrcu_var = (VAR);                                 \
        typeof(VALUE) ovsrcu_value = (VALUE);                           \
        memory_order ovsrcu_order = (ORDER);                            \
                                                                        \
        atomic_store_explicit(&ovsrcu_var->p, ovsrcu_value, ovsrcu_order); \
        (void *) 0;                                                     \
    })
#else  /* not GNU C */
struct ovsrcu_pointer { ATOMIC(void *) p; };
#define OVSRCU_TYPE(TYPE) struct ovsrcu_pointer
#define OVSRCU_INITIALIZER(VALUE) { ATOMIC_VAR_INIT(VALUE) }
static inline void *
ovsrcu_get__(const struct ovsrcu_pointer *pointer, memory_order order)
{
    void *value;
    atomic_read_explicit(&CONST_CAST(struct ovsrcu_pointer *, pointer)->p,
                         &value, order);
    return value;
}
#define ovsrcu_get(TYPE, VAR) \
    CONST_CAST(TYPE, ovsrcu_get__(VAR, memory_order_consume))
#define ovsrcu_get_protected(TYPE, VAR) \
    CONST_CAST(TYPE, ovsrcu_get__(VAR, memory_order_relaxed))

static inline void ovsrcu_set__(struct ovsrcu_pointer *pointer,
                                const void *value,
                                memory_order order)
{
    atomic_store_explicit(&pointer->p, CONST_CAST(void *, value), order);
}
#endif

/* Writes VALUE to the RCU-protected pointer whose address is VAR.
 *
 * Users require external synchronization (e.g. a mutex).  See "Usage" above
 * for an example. */
#define ovsrcu_set(VAR, VALUE) \
    ovsrcu_set__(VAR, VALUE, memory_order_release)

/* This can be used for initializing RCU pointers before any readers can
 * see them.  A later ovsrcu_set() needs to make the bigger structure this
 * is part of visible to the readers. */
#define ovsrcu_set_hidden(VAR, VALUE) \
    ovsrcu_set__(VAR, VALUE, memory_order_relaxed)

/* This can be used for initializing RCU pointers before any readers are
 * executing. */
#define ovsrcu_init(VAR, VALUE) atomic_init(&(VAR)->p, VALUE)

/* Calls FUNCTION passing ARG as its pointer-type argument following the next
 * grace period.  See "Usage" above for an example. */
void ovsrcu_postpone__(void (*function)(void *aux), void *aux);
#define ovsrcu_postpone(FUNCTION, ARG)                          \
    (/* Verify that ARG is appropriate for FUNCTION. */         \
     (void) sizeof((FUNCTION)(ARG), 1),                         \
     /* Verify that ARG is a pointer type. */                   \
     (void) sizeof(*(ARG)),                                     \
     ovsrcu_postpone__((void (*)(void *))(FUNCTION), ARG))

/* An array index protected by RCU semantics.  This is an easier alternative to
 * an RCU protected pointer to a malloc'd int. */
typedef struct { atomic_int v; } ovsrcu_index;

static inline int ovsrcu_index_get__(const ovsrcu_index *i, memory_order order)
{
    int ret;
    atomic_read_explicit(CONST_CAST(atomic_int *, &i->v), &ret, order);
    return ret;
}

/* Returns the index contained in 'i'.  The returned value can be used until
 * the next grace period. */
static inline int ovsrcu_index_get(const ovsrcu_index *i)
{
    return ovsrcu_index_get__(i, memory_order_consume);
}

/* Returns the index contained in 'i'.  This is an alternative to
 * ovsrcu_index_get() that can be used when there's no possible concurrent
 * writer. */
static inline int ovsrcu_index_get_protected(const ovsrcu_index *i)
{
    return ovsrcu_index_get__(i, memory_order_relaxed);
}

static inline void ovsrcu_index_set__(ovsrcu_index *i, int value,
                                      memory_order order)
{
    atomic_store_explicit(&i->v, value, order);
}

/* Writes the index 'value' in 'i'.  The previous value of 'i' may still be
 * used by readers until the next grace period. */
static inline void ovsrcu_index_set(ovsrcu_index *i, int value)
{
    ovsrcu_index_set__(i, value, memory_order_release);
}

/* Writes the index 'value' in 'i'.  This is an alternative to
 * ovsrcu_index_set() that can be used when there's no possible concurrent
 * reader. */
static inline void ovsrcu_index_set_hidden(ovsrcu_index *i, int value)
{
    ovsrcu_index_set__(i, value, memory_order_relaxed);
}

/* Initializes 'i' with 'value'.  This is safe to call as long as there are no
 * concurrent readers. */
static inline void ovsrcu_index_init(ovsrcu_index *i, int value)
{
    atomic_init(&i->v, value);
}

/* Quiescent states. */
void ovsrcu_quiesce_start(void);
void ovsrcu_quiesce_end(void);
void ovsrcu_quiesce(void);
int ovsrcu_try_quiesce(void);
bool ovsrcu_is_quiescent(void);

/* Synchronization.  Waits for all non-quiescent threads to quiesce at least
 * once.  This can block for a relatively long time. */
void ovsrcu_synchronize(void);

void ovsrcu_exit(void);

/* Read-Copy-Update (RCU)
 * ======================
 *
 * 简介
 * ----
 *
 * 原子指针访问使得实现无锁算法变得非常容易。然而，存在一个大问题：当一个写操作更新一个指针以指向一个新的数据结构时，某个线程可能正在读取旧版本的数据，并且没有方便的方法来释放旧版本的数据，因为所有线程都可能还在使用它。
 *
 * ovsrcu_postpone() 函数解决了这个问题。传递给该函数的函数指针只在所有线程都完成对旧版本数据结构的访问后才会被调用。回调函数用于释放不再使用的旧版本数据。这种技术被称为“读取-复制-更新”（Read-Copy-Update），简称为 RCU。
 *
 *
 * 详细说明
 * --------
 *
 * “静态状态”是指线程不持有指向由 RCU 管理的内存的指针时的状态。也就是说，当线程不再引用可能是通过 RCU 释放的某个对象的旧版本内存时，该线程处于静态状态。例如，poll_block() 包含一个静态状态。
 *
 * 以下函数用于管理静态状态的识别：
 *
 *     void ovsrcu_quiesce(void)
 *
 *         在当前线程中识别一个短暂的静态状态。
 *
 *     void ovsrcu_quiesce_start(void)
 *     void ovsrcu_quiesce_end(void)
 *
 *         用于标记当前线程处于静态状态的时间段。
 *
 * 新创建的线程最初是活动的，不处于静态状态。当进程变为多线程时，主线程变为活动状态，不处于静态状态。
 *
 * 当每个线程都发生了静态状态时，我们称之为“优雅周期”（grace period）。在优雅周期之后，所有在优雅周期开始之前延迟的回调函数都可能被调用。OVS 通过 RCU 机制自动处理这一点：当进程仍然只有一个线程时，它会从 ovsrcu_quiesce() 和 ovsrcu_quiesce_start() 直接调用延迟的回调函数；当创建了额外的辅助线程后，它会创建一个额外的辅助线程来调用回调函数。
 *
 * 请注意，尽管延迟的函数调用保证在下一次所有参与线程至少一次静态化之后发生，但无法保证所有延迟的函数尽早调用，也无法保证不同线程延迟的函数调用按照注册的顺序调用。特别是，即使两个线程可证明以特定顺序延迟一个函数调用，延迟的函数调用仍然可能以相反的顺序调用，这取决于线程何时调用 ovsrcu_quiesce()、它们延迟了多少个函数以及 ovs-rcu 线程在何时抓取要调用的函数。
 *
 * 但是，单个线程延迟的所有函数都保证按照它们被延迟的顺序执行。
 *
 * 使用方法
 * -------
 *
 * 使用 OVSRCU_TYPE(TYPE) 声明一个指向受 RCU 保护的数据的指针，例如下面声明一个名为 flowp 的受 RCU 保护的 "struct flow *"：
 *
 *     OVSRCU_TYPE(struct flow *) flowp;
 *
 * 使用 ovsrcu_get(TYPE, VAR) 读取受 RCU 保护的指针，例如读取上面声明的指针变量：
 *
 *     struct flow *flow = ovsrcu_get(struct flow *, &flowp);
 *
 * 如果当前线程持有保护该指针的互斥锁，使其免受更改的影响，则可以使用 ovsrcu_get_protected()。在 Alpha 架构上，这可能会生成不同的代码，但它可能是有用的文档。
 *
 * （对于 GNU C 或 Clang，如果 TYPE 错误，会产生编译错误；其他编译器则会继续接受错误的类型。）
 *
 * 使用 ovsrcu_set() 写入受 RCU 保护的指针，并使用 ovsrcu_postpone() 释放先前的数据。ovsrcu_set_hidden() 可用于尚不对任何读取器可见的受 RCU 保护的数据，但将在稍后的 ovsrcu_set() 中可见。ovsrcu_init() 可用于在没有读取器执行的情况下初始化 RCU 指针。如果多个线程可以写入指针，则需要某种形式的外部同步（例如互斥锁）来防止写入者相互干扰。例如，安全地写入上面声明的指针变量并释放旧值的示例：
 *
 *     static struct ovs_mutex mutex = OVS_MUTEX_INITIALIZER;
 *
 *     OVSRCU_TYPE(struct flow *) flowp;
 *
 *     void
 *     change_flow(struct flow *new_flow)
 *     {
 *         ovs_mutex_lock(&mutex);
 *         ovsrcu_postpone(free,
 *                         ovsrcu_get_protected(struct flow *, &flowp));
 *         ovsrcu_set(&flowp, new_flow);
 *         ovs_mutex_unlock(&mutex);
 *     }
 *
 * 在一些罕见的情况下，对象可能不能通过指针寻址，而只能通过数组索引访问（例如，因为它是由另一个库提供的）。然而，仍然可以通过使用 ovsrcu_index 类型来实现 RCU 语义。
 *
 *     static struct ovs_mutex mutex = OVS_MUTEX_INITIALIZER;
 *
 *     ovsrcu_index port_id;
 *
 *     void tx()
 *     {
 *         int id = ovsrcu_index_get(&port_id);
 *         if (id == -1) {
 *             return;
 *         }
 *         port_tx(id);
 *     }
 *
 *     void delete()
 *     {
 *         int id;
 *
 *         ovs_mutex_lock(&mutex);
 *         id = ovsrcu_index_get_protected(&port_id);
 *         ovsrcu_index_set(&port_id, -1);
 *         ovs_mutex_unlock(&mutex);
 *
 *         ovsrcu_synchronize();
 *         port_delete(id);
 *     }
 *
 * 使用 ovsrcu_barrier() 等待所有未完成的 RCU 回调完成。当您必须销毁某些资源时，这非常有用，但这些资源在未完成的 RCU 回调中被引用。
 *
 *     void rcu_cb(void *A) {
 *         do_something(A);
 *     }
 *
 *     void destroy_A() {
 *         ovsrcu_postpone(rcu_cb, A); // 将在稍后使用 A
 *         ovsrcu_barrier(); // 等待 rcu_cb 完成
 *         do_destroy_A(); // 释放 A
 *     }
 */

#endif /* ovs-rcu.h */
