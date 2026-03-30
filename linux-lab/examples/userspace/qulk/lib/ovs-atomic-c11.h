/* This header implements atomic operation primitives on compilers that
 * have built-in support for C11 <stdatomic.h>  */
#ifndef IN_OVS_ATOMIC_H
#error "This header should only be included indirectly via ovs-atomic.h."
#endif

#include <stdatomic.h>

#define OMIT_STANDARD_ATOMIC_TYPES 1
#define ATOMIC(TYPE) _Atomic(TYPE)

#define atomic_read(SRC, DST) \
    atomic_read_explicit(SRC, DST, memory_order_seq_cst)
#define atomic_read_explicit(SRC, DST, ORDER)   \
    (*(DST) = atomic_load_explicit(SRC, ORDER), \
     (void) 0)

#define atomic_add(RMW, ARG, ORIG) \
    atomic_add_explicit(RMW, ARG, ORIG, memory_order_seq_cst)
#define atomic_sub(RMW, ARG, ORIG) \
    atomic_sub_explicit(RMW, ARG, ORIG, memory_order_seq_cst)
#define atomic_or(RMW, ARG, ORIG) \
    atomic_or_explicit(RMW, ARG, ORIG, memory_order_seq_cst)
#define atomic_xor(RMW, ARG, ORIG) \
    atomic_xor_explicit(RMW, ARG, ORIG, memory_order_seq_cst)
#define atomic_and(RMW, ARG, ORIG) \
    atomic_and_explicit(RMW, ARG, ORIG, memory_order_seq_cst)

#define atomic_add_explicit(RMW, ARG, ORIG, ORDER) \
    (*(ORIG) = atomic_fetch_add_explicit(RMW, ARG, ORDER), (void) 0)
#define atomic_sub_explicit(RMW, ARG, ORIG, ORDER) \
    (*(ORIG) = atomic_fetch_sub_explicit(RMW, ARG, ORDER), (void) 0)
#define atomic_or_explicit(RMW, ARG, ORIG, ORDER) \
    (*(ORIG) = atomic_fetch_or_explicit(RMW, ARG, ORDER), (void) 0)
#define atomic_xor_explicit(RMW, ARG, ORIG, ORDER) \
    (*(ORIG) = atomic_fetch_xor_explicit(RMW, ARG, ORDER), (void) 0)
#define atomic_and_explicit(RMW, ARG, ORIG, ORDER) \
    (*(ORIG) = atomic_fetch_and_explicit(RMW, ARG, ORDER), (void) 0)
#if 0
#define atomic_exchange_explicit(RMW, ARG, ORIG, ORDER) \
    (*(ORIG) = atomic_exchange_explicit(RMW, ARG, ORDER), (void) 0)
#endif
