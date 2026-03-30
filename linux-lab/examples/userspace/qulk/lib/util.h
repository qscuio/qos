#ifndef __HAVE_UTIL_H__
#define __HAVE_UTIL_H__

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "types.h"
#include "dynamic-string.h"
#include "unaligned.h"
#include "linklist.h"
#include "commsg.h"
#include "cqueue.h"
#if defined(__aarch64__) && __GNUC__ >= 6
#include <arm_neon.h>
#endif

struct abc
{
	struct thread_master *master;
	COMMSG *commsg;
	module_id_t protocol;
	struct list *ssock_cb_zombie_list;
	struct cqueue_buf_list *cqueue_buf_free_list;
};

extern char *program_name;
void ovs_set_program_name(const char *name, const char *version);

const char *ovs_get_program_name(void);
const char *ovs_get_program_version(void);

/* Expands to a string that looks like "<file>:<line>", e.g. "tmp.c:10".
 *
 * See http://c-faq.com/ansi/stringize.html for an explanation of OVS_STRINGIZE
 * and OVS_STRINGIZE2. */
#define OVS_SOURCE_LOCATOR __FILE__ ":" OVS_STRINGIZE(__LINE__)
#define OVS_STRINGIZE(ARG) OVS_STRINGIZE2(ARG)
#define OVS_STRINGIZE2(ARG) #ARG

/* Saturating multiplication of "unsigned int"s: overflow yields UINT_MAX. */
#define OVS_SAT_MUL(X, Y)                                               \
    ((Y) == 0 ? 0                                                       \
     : (X) <= UINT_MAX / (Y) ? (unsigned int) (X) * (unsigned int) (Y)  \
     : UINT_MAX)

/* Like the standard assert macro, except:
 *
 *    - Writes the failure message to the log.
 *
 *    - Always evaluates the condition, even with NDEBUG. */
#ifndef NDEBUG
#define ovs_assert(CONDITION)                                           \
    (OVS_LIKELY(CONDITION)                                              \
     ? (void) 0                                                         \
     : ovs_assert_failure(OVS_SOURCE_LOCATOR, __func__, #CONDITION))
#else
#define ovs_assert(CONDITION) ((void) (CONDITION))
#endif

OVS_NO_RETURN void ovs_assert_failure(const char *, const char *, const char *);

/* This is a void expression that issues a compiler error if POINTER cannot be
 * compared for equality with the given pointer TYPE.  This generally means
 * that POINTER is a qualified or unqualified TYPE.  However,
 * BUILD_ASSERT_TYPE(POINTER, void *) will accept any pointer to object type,
 * because any pointer to object can be compared for equality with "void *".
 *
 * POINTER can be any expression.  The use of "sizeof" ensures that the
 * expression is not actually evaluated, so that any side effects of the
 * expression do not occur.
 *
 * The cast to int is present only to suppress an "expression using sizeof
 * bool" warning from "sparse" (see
 * http://permalink.gmane.org/gmane.comp.parsers.sparse/2967). */
#define BUILD_ASSERT_TYPE(POINTER, TYPE) \
    ((void) sizeof ((int) ((POINTER) == (TYPE) (POINTER))))

/* Casts 'pointer' to 'type' and issues a compiler warning if the cast changes
 * anything other than an outermost "const" or "volatile" qualifier. */
#define CONST_CAST(TYPE, POINTER)                               \
    (BUILD_ASSERT_TYPE(POINTER, TYPE),                          \
     (TYPE) (POINTER))

/* Given a pointer-typed lvalue OBJECT, expands to a pointer type that may be
 * assigned to OBJECT. */
#ifdef __GNUC__
#define OVS_TYPEOF(OBJECT) typeof(OBJECT)
#else
#define OVS_TYPEOF(OBJECT) void *
#endif

/* Given OBJECT of type pointer-to-structure, expands to the offset of MEMBER
 * within an instance of the structure.
 *
 * The GCC-specific version avoids the technicality of undefined behavior if
 * OBJECT is null, invalid, or not yet initialized.  This makes some static
 * checkers (like Coverity) happier.  But the non-GCC version does not actually
 * dereference any pointer, so it would be surprising for it to cause any
 * problems in practice.
 */
#ifdef __GNUC__
#define OBJECT_OFFSETOF(OBJECT, MEMBER) offsetof(typeof(*(OBJECT)), MEMBER)
#else
#define OBJECT_OFFSETOF(OBJECT, MEMBER) \
    ((char *) &(OBJECT)->MEMBER - (char *) (OBJECT))
#endif

/* Yields the size of MEMBER within STRUCT. */
#define MEMBER_SIZEOF(STRUCT, MEMBER) (sizeof(((STRUCT *) NULL)->MEMBER))

/* Yields the offset of the end of MEMBER within STRUCT. */
#define OFFSETOFEND(STRUCT, MEMBER) \
        (offsetof(STRUCT, MEMBER) + MEMBER_SIZEOF(STRUCT, MEMBER))

/* Given POINTER, the address of the given MEMBER in a STRUCT object, returns
   the STRUCT object. */
#define CONTAINER_OF(POINTER, STRUCT, MEMBER)                           \
        ((STRUCT *) (void *) ((char *) (POINTER) - offsetof (STRUCT, MEMBER)))

/* Given POINTER, the address of the given MEMBER within an object of the type
 * that that OBJECT points to, returns OBJECT as an assignment-compatible
 * pointer type (either the correct pointer type or "void *").  OBJECT must be
 * an lvalue.
 *
 * This is the same as CONTAINER_OF except that it infers the structure type
 * from the type of '*OBJECT'. */
#define OBJECT_CONTAINING(POINTER, OBJECT, MEMBER)                      \
    ((OVS_TYPEOF(OBJECT)) (void *)                                      \
     ((char *) (POINTER) - OBJECT_OFFSETOF(OBJECT, MEMBER)))

/* Given POINTER, the address of the given MEMBER within an object of the type
 * that that OBJECT points to, assigns the address of the outer object to
 * OBJECT, which must be an lvalue.
 *
 * Evaluates to (void) 0 as the result is not to be used. */
#define ASSIGN_CONTAINER(OBJECT, POINTER, MEMBER) \
    ((OBJECT) = OBJECT_CONTAINING(POINTER, OBJECT, MEMBER), (void) 0)

/* As explained in the comment above OBJECT_OFFSETOF(), non-GNUC compilers
 * like MSVC will complain about un-initialized variables if OBJECT
 * hasn't already been initialized. To prevent such warnings, INIT_CONTAINER()
 * can be used as a wrapper around ASSIGN_CONTAINER. */
#define INIT_CONTAINER(OBJECT, POINTER, MEMBER) \
    ((OBJECT) = NULL, ASSIGN_CONTAINER(OBJECT, POINTER, MEMBER))

/* Returns the number of elements in ARRAY. */
#define ARRAY_SIZE(ARRAY) __ARRAY_SIZE(ARRAY)

/* Returns X / Y, rounding up.  X must be nonnegative to round correctly. */
#define DIV_ROUND_UP(X, Y) (((X) + ((Y) - 1)) / (Y))

/* Returns X rounded up to the nearest multiple of Y. */
#define ROUND_UP(X, Y) (DIV_ROUND_UP(X, Y) * (Y))

/* Returns the least number that, when added to X, yields a multiple of Y. */
#define PAD_SIZE(X, Y) (ROUND_UP(X, Y) - (X))

/* Returns X rounded down to the nearest multiple of Y. */
#define ROUND_DOWN(X, Y) ((X) / (Y) * (Y))

/* Returns true if X is a power of 2, otherwise false. */
#define IS_POW2(X) ((X) && !((X) & ((X) - 1)))

/* Expands to an anonymous union that contains:
 *
 *    - MEMBERS in a nested anonymous struct.
 *
 *    - An array as large as MEMBERS plus padding to a multiple of UNIT bytes.
 *
 * The effect is to pad MEMBERS to a multiple of UNIT bytes.
 *
 * For example, the struct below is 8 bytes long, with 6 bytes of padding:
 *
 *     struct padded_struct {
 *         PADDED_MEMBERS(8, uint8_t x; uint8_t y;);
 *     };
 */
#define PAD_PASTE2(x, y) x##y
#define PAD_PASTE(x, y) PAD_PASTE2(x, y)
#define PAD_ID PAD_PASTE(pad, __COUNTER__)
#define PADDED_MEMBERS(UNIT, MEMBERS)                               \
    union {                                                         \
        struct { MEMBERS };                                         \
        uint8_t PAD_ID[ROUND_UP(sizeof(struct { MEMBERS }), UNIT)]; \
    }

/* Similar to PADDED_MEMBERS with additional cacheline marker:
 *
 *    - OVS_CACHE_LINE_MARKER is a cacheline marker
 *    - MEMBERS in a nested anonymous struct.
 *    - An array as large as MEMBERS plus padding to a multiple of UNIT bytes.
 *
 * The effect is to add cacheline marker and pad MEMBERS to a multiple of
 * UNIT bytes.
 *
 * Example:
 *     struct padded_struct {
 *         PADDED_MEMBERS_CACHELINE_MARKER(CACHE_LINE_SIZE, cacheline0,
 *             uint8_t x;
 *             uint8_t y;
 *         );
 *     };
 *
 * The PADDED_MEMBERS_CACHELINE_MARKER macro in above structure expands as:
 *
 *     struct padded_struct {
 *            union {
 *                    OVS_CACHE_LINE_MARKER cacheline0;
 *                    struct {
 *                            uint8_t x;
 *                            uint8_t y;
 *                    };
 *                    uint8_t         pad0[64];
 *            };
 *            *--- cacheline 1 boundary (64 bytes) ---*
 *     };
 */
#define PADDED_MEMBERS_CACHELINE_MARKER(UNIT, CACHELINE, MEMBERS)   \
    union {                                                         \
        OVS_CACHE_LINE_MARKER CACHELINE;                            \
        struct { MEMBERS };                                         \
        uint8_t PAD_ID[ROUND_UP(sizeof(struct { MEMBERS }), UNIT)]; \
    }

static inline bool
is_pow2(uintmax_t x)
{
    return IS_POW2(x);
}

/* Returns X rounded up to a power of 2.  X must be a constant expression. */
#define ROUND_UP_POW2(X) RUP2__(X)
#define RUP2__(X) (RUP2_1(X) + 1)
#define RUP2_1(X) (RUP2_2(X) | (RUP2_2(X) >> 16))
#define RUP2_2(X) (RUP2_3(X) | (RUP2_3(X) >> 8))
#define RUP2_3(X) (RUP2_4(X) | (RUP2_4(X) >> 4))
#define RUP2_4(X) (RUP2_5(X) | (RUP2_5(X) >> 2))
#define RUP2_5(X) (RUP2_6(X) | (RUP2_6(X) >> 1))
#define RUP2_6(X) ((X) - 1)

/* Returns X rounded down to a power of 2.  X must be a constant expression. */
#define ROUND_DOWN_POW2(X) RDP2__(X)
#define RDP2__(X) (RDP2_1(X) - (RDP2_1(X) >> 1))
#define RDP2_1(X) (RDP2_2(X) | (RDP2_2(X) >> 16))
#define RDP2_2(X) (RDP2_3(X) | (RDP2_3(X) >> 8))
#define RDP2_3(X) (RDP2_4(X) | (RDP2_4(X) >> 4))
#define RDP2_4(X) (RDP2_5(X) | (RDP2_5(X) >> 2))
#define RDP2_5(X) (      (X) | (      (X) >> 1))

/* Macros for sizing bitmaps */
#define BITMAP_ULONG_BITS (sizeof(unsigned long) * CHAR_BIT)
#define BITMAP_N_LONGS(N_BITS) DIV_ROUND_UP(N_BITS, BITMAP_ULONG_BITS)

/* Given ATTR, and TYPE, cast the ATTR to TYPE by first casting ATTR to (void
 * *).  This suppresses the alignment warning issued by Clang and newer
 * versions of GCC when a pointer is cast to a type with a stricter alignment.
 *
 * Add ALIGNED_CAST only if you are sure that the cast is actually correct,
 * that is, that the pointer is actually properly aligned for the stricter
 * type.  On RISC architectures, dereferencing a misaligned pointer can cause a
 * segfault, so it is important to be aware of correct alignment. */
#define ALIGNED_CAST(TYPE, ATTR) ((TYPE) (void *) (ATTR))



#define __ARRAY_SIZE_NOCHECK(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))
/* return 0 for array types, 1 otherwise */
#define __ARRAY_CHECK(ARRAY) 					\
    !__builtin_types_compatible_p(typeof(ARRAY), typeof(&ARRAY[0]))

/* compile-time fail if not array */
#define __ARRAY_FAIL(ARRAY) (sizeof(char[-2*!__ARRAY_CHECK(ARRAY)]))
#define __ARRAY_SIZE(ARRAY)					\
    __builtin_choose_expr(__ARRAY_CHECK(ARRAY),			\
        __ARRAY_SIZE_NOCHECK(ARRAY), __ARRAY_FAIL(ARRAY))

/* This system's cache line size, in bytes.
 * Being wrong hurts performance but not correctness. */
#define CACHE_LINE_SIZE 64
BUILD_ASSERT_DECL(IS_POW2(CACHE_LINE_SIZE));

/* Cacheline marking is typically done using zero-sized array.
 * However MSVC doesn't like zero-sized array in struct/union.
 * C4200: https://msdn.microsoft.com/en-us/library/79wf64bc.aspx
 */
typedef uint8_t OVS_CACHE_LINE_MARKER[1];

static inline void
ovs_prefetch_range(const void *start, size_t size)
{
    const char *addr = (const char *)start;
    size_t ofs;

    for (ofs = 0; ofs < size; ofs += CACHE_LINE_SIZE) {
        OVS_PREFETCH(addr + ofs);
    }
}

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

/* Comparisons for ints with modular arithmetic */
#define INT_MOD_LT(a,b)     ((int) ((a)-(b)) < 0)
#define INT_MOD_LEQ(a,b)    ((int) ((a)-(b)) <= 0)
#define INT_MOD_GT(a,b)     ((int) ((a)-(b)) > 0)
#define INT_MOD_GEQ(a,b)    ((int) ((a)-(b)) >= 0)

#define INT_MOD_MIN(a, b)   ((INT_MOD_LT(a, b)) ? (a) : (b))
#define INT_MOD_MAX(a, b)   ((INT_MOD_GT(a, b)) ? (a) : (b))

#define OVS_NOT_REACHED() abort()

/* Joins two token expanding the arguments if they are macros.
 *
 * For token concatenation the circumlocution is needed for the
 * expansion. */
#define OVS_JOIN2(X, Y) X##Y
#define OVS_JOIN(X, Y) OVS_JOIN2(X, Y)

/* Use "%"PRIuSIZE to format size_t with printf(). */
#define PRIdSIZE "zd"
#define PRIiSIZE "zi"
#define PRIoSIZE "zo"
#define PRIuSIZE "zu"
#define PRIxSIZE "zx"
#define PRIXSIZE "zX"

typedef uint32_t HANDLE;

#define set_program_name(name) \
        ovs_set_program_name(name, OVS_PACKAGE_VERSION)

const char *get_subprogram_name(void);
    void set_subprogram_name(const char *);

unsigned int get_page_size(void);
long long int get_boot_time(void);

void ctl_timeout_setup(unsigned int secs);

void ovs_print_version(uint8_t min_ofp, uint8_t max_ofp);

void set_memory_locked(void);
bool memory_locked(void);

OVS_NO_RETURN void out_of_memory(void);

/* Allocation wrappers that abort if memory is exhausted. */
void *xmalloc(size_t) MALLOC_LIKE;
void *xcalloc(size_t, size_t) MALLOC_LIKE;
void *xzalloc(size_t) MALLOC_LIKE;
void *xrealloc(void *, size_t);
void *xmemdup(const void *, size_t) MALLOC_LIKE;
char *xmemdup0(const char *, size_t) MALLOC_LIKE;
char *xstrdup(const char *) MALLOC_LIKE;
char *nullable_xstrdup(const char *) MALLOC_LIKE;
bool nullable_string_is_equal(const char *a, const char *b);
char *xasprintf(const char *format, ...) OVS_PRINTF_FORMAT(1, 2) MALLOC_LIKE;
char *xvasprintf(const char *format, va_list) OVS_PRINTF_FORMAT(1, 0) MALLOC_LIKE;
void *x2nrealloc(void *p, size_t *n, size_t s);

/* Allocation wrappers for specialized situations where coverage counters
 * cannot be used. */
void *xmalloc__(size_t) MALLOC_LIKE;
void *xcalloc__(size_t, size_t) MALLOC_LIKE;
void *xzalloc__(size_t) MALLOC_LIKE;
void *xrealloc__(void *, size_t);

void *xmalloc_cacheline(size_t) MALLOC_LIKE;
void *xzalloc_cacheline(size_t) MALLOC_LIKE;
void free_cacheline(void *);

void ovs_strlcpy(char *dst, const char *src, size_t size);
void ovs_strzcpy(char *dst, const char *src, size_t size);

int string_ends_with(const char *str, const char *suffix);

void *xmalloc_pagealign(size_t) MALLOC_LIKE;
void free_pagealign(void *);
void *xmalloc_size_align(size_t, size_t) MALLOC_LIKE;
void free_size_align(void *);

/* The C standards say that neither the 'dst' nor 'src' argument to
 * memcpy() may be null, even if 'n' is zero.  This wrapper tolerates
 * the null case. */
static inline void
nullable_memcpy(void *dst, const void *src, size_t n)
{
    if (n) {
        memcpy(dst, src, n);
    }
}

/* The C standards say that the 'dst' argument to memset may not be
 * null, even if 'n' is zero.  This wrapper tolerates the null case. */
static inline void
nullable_memset(void *dst, int c, size_t n)
{
    if (n) {
        memset(dst, c, n);
    }
}

/* Copy string SRC to DST, but no more bytes than the shorter of DST or SRC.
 * DST and SRC must both be char arrays, not pointers, and with GNU C, this
 * raises a compiler error if either DST or SRC is a pointer instead of an
 * array. */
#define ovs_strlcpy_arrays(DST, SRC) \
    ovs_strlcpy(DST, SRC, MIN(ARRAY_SIZE(DST), ARRAY_SIZE(SRC)))

OVS_NO_RETURN void ovs_abort(int err_no, const char *format, ...)
    OVS_PRINTF_FORMAT(2, 3);
OVS_NO_RETURN void ovs_abort_valist(int err_no, const char *format, va_list)
    OVS_PRINTF_FORMAT(2, 0);
OVS_NO_RETURN void ovs_fatal(int err_no, const char *format, ...)
    OVS_PRINTF_FORMAT(2, 3);
OVS_NO_RETURN void ovs_fatal_valist(int err_no, const char *format, va_list)
    OVS_PRINTF_FORMAT(2, 0);
void ovs_error(int err_no, const char *format, ...) OVS_PRINTF_FORMAT(2, 3);
void ovs_error_valist(int err_no, const char *format, va_list)
    OVS_PRINTF_FORMAT(2, 0);
const char *ovs_retval_to_string(int);
const char *ovs_strerror(int);
void ovs_hex_dump(FILE *, const void *, size_t, uintptr_t offset, bool ascii);

bool str_to_int(const char *, int base, int *);
bool str_to_long(const char *, int base, long *);
bool str_to_llong(const char *, int base, long long *);
bool str_to_llong_with_tail(const char *, char **, int base, long long *);
bool str_to_uint(const char *, int base, unsigned int *);
bool str_to_ullong(const char *, int base, unsigned long long *);
bool str_to_llong_range(const char *, int base, long long *, long long *);

bool ovs_scan(const char *s, const char *format, ...) OVS_SCANF_FORMAT(2, 3);
bool ovs_scan_len(const char *s, int *n, const char *format, ...);

bool str_to_double(const char *, double *);

int hexit_value(unsigned char c);
uintmax_t hexits_value(const char *s, size_t n, bool *ok);

int parse_int_string(const char *s, uint8_t *valuep, int field_width,
                     char **tail);

const char *english_list_delimiter(size_t index, size_t total);

char *get_cwd(void);
char *dir_name(const char *file_name);
char *base_name(const char *file_name);
char *abs_file_name(const char *dir, const char *file_name);
bool is_file_name_absolute(const char *);

char *follow_symlinks(const char *filename);

void ignore(bool x OVS_UNUSED);

/* Bitwise tests. */

/* Returns the number of trailing 0-bits in 'n'.  Undefined if 'n' == 0. */
#if __GNUC__ >= 4
static inline int
raw_ctz(uint64_t n)
{
    /* With GCC 4.7 on 32-bit x86, if a 32-bit integer is passed as 'n', using
     * a plain __builtin_ctzll() here always generates an out-of-line function
     * call.  The test below helps it to emit a single 'bsf' instruction. */
    return (__builtin_constant_p(n <= UINT32_MAX) && n <= UINT32_MAX
            ? __builtin_ctz(n)
            : __builtin_ctzll(n));
}

static inline int
raw_clz64(uint64_t n)
{
    return __builtin_clzll(n);
}
#else
/* Defined in util.c. */
int raw_ctz(uint64_t n);
int raw_clz64(uint64_t n);
#endif

/* Returns the number of trailing 0-bits in 'n', or 32 if 'n' is 0. */
static inline int
ctz32(uint32_t n)
{
    return n ? raw_ctz(n) : 32;
}

/* Returns the number of trailing 0-bits in 'n', or 64 if 'n' is 0. */
static inline int
ctz64(uint64_t n)
{
    return n ? raw_ctz(n) : 64;
}

/* Returns the number of leading 0-bits in 'n', or 32 if 'n' is 0. */
static inline int
clz32(uint32_t n)
{
    return n ? raw_clz64(n) - 32 : 32;
}

/* Returns the number of leading 0-bits in 'n', or 64 if 'n' is 0. */
static inline int
clz64(uint64_t n)
{
    return n ? raw_clz64(n) : 64;
}

/* Given a word 'n', calculates floor(log_2('n')).  This is equivalent
 * to finding the bit position of the most significant one bit in 'n'.  It is
 * an error to call this function with 'n' == 0. */
static inline int
log_2_floor(uint64_t n)
{
    return 63 - raw_clz64(n);
}

/* Given a word 'n', calculates ceil(log_2('n')).  It is an error to
 * call this function with 'n' == 0. */
static inline int
log_2_ceil(uint64_t n)
{
    return log_2_floor(n) + !is_pow2(n);
}

/* unsigned int count_1bits(uint64_t x):
 *
 * Returns the number of 1-bits in 'x', between 0 and 64 inclusive. */
#if UINTPTR_MAX == UINT64_MAX
static inline unsigned int
count_1bits(uint64_t x)
{
#if (__GNUC__ >= 4 && __POPCNT__) || (defined(__aarch64__) && __GNUC__ >= 7)
    return __builtin_popcountll(x);
#elif defined(__aarch64__) && __GNUC__ >= 6
    return vaddv_u8(vcnt_u8(vcreate_u8(x)));
#else
    /* This portable implementation is the fastest one we know of for 64
     * bits, and about 3x faster than GCC 4.7 __builtin_popcountll(). */
    const uint64_t h55 = UINT64_C(0x5555555555555555);
    const uint64_t h33 = UINT64_C(0x3333333333333333);
    const uint64_t h0F = UINT64_C(0x0F0F0F0F0F0F0F0F);
    const uint64_t h01 = UINT64_C(0x0101010101010101);
    x -= (x >> 1) & h55;               /* Count of each 2 bits in-place. */
    x = (x & h33) + ((x >> 2) & h33);  /* Count of each 4 bits in-place. */
    x = (x + (x >> 4)) & h0F;          /* Count of each 8 bits in-place. */
    return (x * h01) >> 56;            /* Sum of all bytes. */
#endif
}
#else /* Not 64-bit. */
#if __GNUC__ >= 4 && __POPCNT__
static inline unsigned int
count_1bits_32__(uint32_t x)
{
    return __builtin_popcount(x);
}
#else
#define NEED_COUNT_1BITS_8 1
extern const uint8_t count_1bits_8[256];
static inline unsigned int
count_1bits_32__(uint32_t x)
{
    /* This portable implementation is the fastest one we know of for 32 bits,
     * and faster than GCC __builtin_popcount(). */
    return (count_1bits_8[x & 0xff] +
            count_1bits_8[(x >> 8) & 0xff] +
            count_1bits_8[(x >> 16) & 0xff] +
            count_1bits_8[x >> 24]);
}
#endif
static inline unsigned int
count_1bits(uint64_t x)
{
    return count_1bits_32__(x) + count_1bits_32__(x >> 32);
}
#endif

/* Returns the rightmost 1-bit in 'x' (e.g. 01011000 => 00001000), or 0 if 'x'
 * is 0. */
static inline uintmax_t
rightmost_1bit(uintmax_t x)
{
    return x & -x;
}

/* Returns 'x' with its rightmost 1-bit changed to a zero (e.g. 01011000 =>
 * 01010000), or 0 if 'x' is 0. */
static inline uintmax_t
zero_rightmost_1bit(uintmax_t x)
{
    return x & (x - 1);
}

/* Returns the index of the rightmost 1-bit in 'x' (e.g. 01011000 => 3), or an
 * undefined value if 'x' is 0. */
static inline int
rightmost_1bit_idx(uint64_t x)
{
    return ctz64(x);
}

/* Returns the index of the leftmost 1-bit in 'x' (e.g. 01011000 => 6), or an
 * undefined value if 'x' is 0. */
static inline uint32_t
leftmost_1bit_idx(uint64_t x)
{
    return log_2_floor(x);
}

/* Return a ovs_be32 prefix in network byte order with 'plen' highest bits set.
 * Shift with 32 is undefined behavior, but we rather use 64-bit shift than
 * compare. */
static inline ovs_be32 be32_prefix_mask(int plen)
{
    return htonl((uint64_t)UINT32_MAX << (32 - plen));
}

/* Returns true if the 1-bits in 'super' are a superset of the 1-bits in 'sub',
 * false otherwise. */
static inline bool
uint_is_superset(uintmax_t super, uintmax_t sub)
{
    return (super & sub) == sub;
}

/* Returns true if the 1-bits in 'super' are a superset of the 1-bits in 'sub',
 * false otherwise. */
static inline bool
be16_is_superset(ovs_be16 super, ovs_be16 sub)
{
    return (super & sub) == sub;
}

/* Returns true if the 1-bits in 'super' are a superset of the 1-bits in 'sub',
 * false otherwise. */
static inline bool
be32_is_superset(ovs_be32 super, ovs_be32 sub)
{
    return (super & sub) == sub;
}

/* Returns true if the 1-bits in 'super' are a superset of the 1-bits in 'sub',
 * false otherwise. */
static inline bool
be64_is_superset(ovs_be64 super, ovs_be64 sub)
{
    return (super & sub) == sub;
}

bool is_all_zeros(const void *, size_t);
bool is_all_ones(const void *, size_t);
bool is_all_byte(const void *, size_t, uint8_t byte);
void or_bytes(void *dst, const void *src, size_t n);
void bitwise_copy(const void *src, unsigned int src_len, unsigned int src_ofs,
                  void *dst, unsigned int dst_len, unsigned int dst_ofs,
                  unsigned int n_bits);
void bitwise_zero(void *dst_, unsigned int dst_len, unsigned dst_ofs,
                  unsigned int n_bits);
void bitwise_one(void *dst_, unsigned int dst_len, unsigned dst_ofs,
                 unsigned int n_bits);
bool bitwise_is_all_zeros(const void *, unsigned int len, unsigned int ofs,
                          unsigned int n_bits);
unsigned int bitwise_scan(const void *, unsigned int len,
                          bool target, unsigned int start, unsigned int end);
int bitwise_rscan(const void *, unsigned int len, bool target,
                  int start, int end);
void bitwise_put(uint64_t value,
                 void *dst, unsigned int dst_len, unsigned int dst_ofs,
                 unsigned int n_bits);
uint64_t bitwise_get(const void *src, unsigned int src_len,
                     unsigned int src_ofs, unsigned int n_bits);
bool bitwise_get_bit(const void *src, unsigned int len, unsigned int ofs);
void bitwise_put0(void *dst, unsigned int len, unsigned int ofs);
void bitwise_put1(void *dst, unsigned int len, unsigned int ofs);
void bitwise_put_bit(void *dst, unsigned int len, unsigned int ofs, bool);
void bitwise_toggle_bit(void *dst, unsigned int len, unsigned int ofs);

/* Returns non-zero if the parameters have equal value. */
static inline int
ovs_u128_equals(const ovs_u128 a, const ovs_u128 b)
{
    return (a.u64.hi == b.u64.hi) && (a.u64.lo == b.u64.lo);
}

/* Returns true if 'val' is 0. */
static inline bool
ovs_u128_is_zero(const ovs_u128 val)
{
    return !(val.u64.hi || val.u64.lo);
}

/* Returns true if 'val' is all ones. */
static inline bool
ovs_u128_is_ones(const ovs_u128 val)
{
    return ovs_u128_equals(val, OVS_U128_MAX);
}

/* Returns non-zero if the parameters have equal value. */
static inline int
ovs_be128_equals(const ovs_be128 a, const ovs_be128 b)
{
    return (a.be64.hi == b.be64.hi) && (a.be64.lo == b.be64.lo);
}

/* Returns true if 'val' is 0. */
static inline bool
ovs_be128_is_zero(const ovs_be128 val)
{
    return !(val.be64.hi || val.be64.lo);
}

static inline ovs_u128
ovs_u128_and(const ovs_u128 a, const ovs_u128 b)
{
    ovs_u128 dst;

    dst.u64.hi = a.u64.hi & b.u64.hi;
    dst.u64.lo = a.u64.lo & b.u64.lo;

    return dst;
}

static inline bool
ovs_be128_is_superset(ovs_be128 super, ovs_be128 sub)
{
    return (be64_is_superset(super.be64.hi, sub.be64.hi) &&
            be64_is_superset(super.be64.lo, sub.be64.lo));
}

static inline bool
ovs_u128_is_superset(ovs_u128 super, ovs_u128 sub)
{
    return (uint_is_superset(super.u64.hi, sub.u64.hi) &&
            uint_is_superset(super.u64.lo, sub.u64.lo));
}

void xsleep(unsigned int seconds);
void xnanosleep(uint64_t nanoseconds);

bool is_stdout_a_tty(void);


/* Example:
 *
 * struct eth_addr mac;
 *    [...]
 * printf("The Ethernet address is "ETH_ADDR_FMT"\n", ETH_ADDR_ARGS(mac));
 *
 */
#define ETH_ADDR_FMT                                                    \
    "%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8
#define ETH_ADDR_ARGS(EA) ETH_ADDR_BYTES_ARGS((EA).ea)
#define ETH_ADDR_BYTES_ARGS(EAB) \
         (EAB)[0], (EAB)[1], (EAB)[2], (EAB)[3], (EAB)[4], (EAB)[5]
#define ETH_ADDR_STRLEN 17

/* Example:
 *
 * struct eth_addr64 eui64;
 *    [...]
 * printf("The EUI-64 address is "ETH_ADDR64_FMT"\n", ETH_ADDR64_ARGS(mac));
 *
 */
#define ETH_ADDR64_FMT \
    "%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":" \
    "%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8
#define ETH_ADDR64_ARGS(EA) ETH_ADDR64_BYTES_ARGS((EA).ea64)
#define ETH_ADDR64_BYTES_ARGS(EAB) \
         (EAB)[0], (EAB)[1], (EAB)[2], (EAB)[3], \
         (EAB)[4], (EAB)[5], (EAB)[6], (EAB)[7]
#define ETH_ADDR64_STRLEN 23

/* Example:
 *
 * char *string = "1 00:11:22:33:44:55 2";
 * struct eth_addr mac;
 * int a, b;
 *
 * if (ovs_scan(string, "%d"ETH_ADDR_SCAN_FMT"%d",
 *              &a, ETH_ADDR_SCAN_ARGS(mac), &b)) {
 *     ...
 * }
 */
#define ETH_ADDR_SCAN_FMT "%"SCNx8":%"SCNx8":%"SCNx8":%"SCNx8":%"SCNx8":%"SCNx8
#define ETH_ADDR_SCAN_ARGS(EA) \
    &(EA).ea[0], &(EA).ea[1], &(EA).ea[2], &(EA).ea[3], &(EA).ea[4], &(EA).ea[5]

#define ETH_TYPE_IP            0x0800
#define ETH_TYPE_ARP           0x0806
#define ETH_TYPE_TEB           0x6558
#define ETH_TYPE_VLAN_8021Q    0x8100
#define ETH_TYPE_VLAN          ETH_TYPE_VLAN_8021Q
#define ETH_TYPE_VLAN_8021AD   0x88a8
#define ETH_TYPE_IPV6          0x86dd
#define ETH_TYPE_LACP          0x8809
#define ETH_TYPE_RARP          0x8035
#define ETH_TYPE_MPLS          0x8847
#define ETH_TYPE_MPLS_MCAST    0x8848
#define ETH_TYPE_NSH           0x894f
#define ETH_TYPE_ERSPAN1       0x88be   /* version 1 type II */
#define ETH_TYPE_ERSPAN2       0x22eb   /* version 2 type III */

static inline bool eth_type_mpls(ovs_be16 eth_type)
{
    return eth_type == htons(ETH_TYPE_MPLS) ||
        eth_type == htons(ETH_TYPE_MPLS_MCAST);
}

static inline bool eth_type_vlan(ovs_be16 eth_type)
{
    return eth_type == htons(ETH_TYPE_VLAN_8021Q) ||
        eth_type == htons(ETH_TYPE_VLAN_8021AD);
}

#define IP_FMT "%"PRIu32".%"PRIu32".%"PRIu32".%"PRIu32
#define IP_ARGS(ip)                             \
    ntohl(ip) >> 24,                            \
    (ntohl(ip) >> 16) & 0xff,                   \
    (ntohl(ip) >> 8) & 0xff,                    \
    ntohl(ip) & 0xff

/* Example:
 *
 * char *string = "1 33.44.55.66 2";
 * ovs_be32 ip;
 * int a, b;
 *
 * if (ovs_scan(string, "%d"IP_SCAN_FMT"%d", &a, IP_SCAN_ARGS(&ip), &b)) {
 *     ...
 * }
 */
#define IP_SCAN_FMT "%"SCNu8".%"SCNu8".%"SCNu8".%"SCNu8
#define IP_SCAN_ARGS(ip)                                    \
        ((void) (ovs_be32) *(ip), &((uint8_t *) ip)[0]),    \
        &((uint8_t *) ip)[1],                               \
        &((uint8_t *) ip)[2],                               \
        &((uint8_t *) ip)[3]

#define IP_PORT_SCAN_FMT "%"SCNu8".%"SCNu8".%"SCNu8".%"SCNu8":%"SCNu16
#define IP_PORT_SCAN_ARGS(ip, port)                                    \
        ((void) (ovs_be32) *(ip), &((uint8_t *) ip)[0]),    \
        &((uint8_t *) ip)[1],                               \
        &((uint8_t *) ip)[2],                               \
        &((uint8_t *) ip)[3],                               \
        ((void) (ovs_be16) *(port), (uint16_t *) port)

/* Returns true if 'netmask' is a CIDR netmask, that is, if it consists of N
 * high-order 1-bits and 32-N low-order 0-bits. */
static inline bool
ip_is_cidr(ovs_be32 netmask)
{
    uint32_t x = ~ntohl(netmask);
    return !(x & (x + 1));
}
static inline bool
ip_is_multicast(ovs_be32 ip)
{
    return (ip & htonl(0xf0000000)) == htonl(0xe0000000);
}
static inline bool
ip_is_local_multicast(ovs_be32 ip)
{
    return (ip & htonl(0xffffff00)) == htonl(0xe0000000);
}
int ip_count_cidr_bits(ovs_be32 netmask);
void ip_format_masked(ovs_be32 ip, ovs_be32 mask, struct ds *);
bool ip_parse(const char *s, ovs_be32 *ip);
char *ip_parse_port(const char *s, ovs_be32 *ip, ovs_be16 *port)
    OVS_WARN_UNUSED_RESULT;
char *ip_parse_masked(const char *s, ovs_be32 *ip, ovs_be32 *mask)
    OVS_WARN_UNUSED_RESULT;
char *ip_parse_cidr(const char *s, ovs_be32 *ip, unsigned int *plen)
    OVS_WARN_UNUSED_RESULT;
char *ip_parse_masked_len(const char *s, int *n, ovs_be32 *ip, ovs_be32 *mask)
    OVS_WARN_UNUSED_RESULT;
char *ip_parse_cidr_len(const char *s, int *n, ovs_be32 *ip,
                        unsigned int *plen)
    OVS_WARN_UNUSED_RESULT;

#define IP_VER(ip_ihl_ver) ((ip_ihl_ver) >> 4)
#define IP_IHL(ip_ihl_ver) ((ip_ihl_ver) & 15)
#define IP_IHL_VER(ihl, ver) (((ver) << 4) | (ihl))

#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif

#ifndef IPPROTO_DCCP
#define IPPROTO_DCCP 33
#endif

#ifndef IPPROTO_IGMP
#define IPPROTO_IGMP 2
#endif

#ifndef IPPROTO_UDPLITE
#define IPPROTO_UDPLITE 136
#endif

/* TOS fields. */
#define IP_ECN_NOT_ECT 0x0
#define IP_ECN_ECT_1 0x01
#define IP_ECN_ECT_0 0x02
#define IP_ECN_CE 0x03
#define IP_ECN_MASK 0x03
#define IP_DSCP_CS6 0xc0
#define IP_DSCP_MASK 0xfc

static inline int
IP_ECN_is_ce(uint8_t dsfield)
{
    return (dsfield & IP_ECN_MASK) == IP_ECN_CE;
}

#define IP_VERSION 4

#define IP_DONT_FRAGMENT  0x4000 /* Don't fragment. */
#define IP_MORE_FRAGMENTS 0x2000 /* More fragments. */
#define IP_FRAG_OFF_MASK  0x1fff /* Fragment offset. */
#define IP_IS_FRAGMENT(ip_frag_off) \
        ((ip_frag_off) & htons(IP_MORE_FRAGMENTS | IP_FRAG_OFF_MASK))

#define IP_HEADER_LEN 20
struct ip_header {
    uint8_t ip_ihl_ver;
    uint8_t ip_tos;
    ovs_be16 ip_tot_len;
    ovs_be16 ip_id;
    ovs_be16 ip_frag_off;
    uint8_t ip_ttl;
    uint8_t ip_proto;
    ovs_be16 ip_csum;
    ovs_16aligned_be32 ip_src;
    ovs_16aligned_be32 ip_dst;
};
BUILD_ASSERT_DECL(IP_HEADER_LEN == sizeof(struct ip_header));

/* ICMPv4 types. */
#define ICMP4_ECHO_REPLY 0
#define ICMP4_DST_UNREACH 3
#define ICMP4_SOURCEQUENCH 4
#define ICMP4_REDIRECT 5
#define ICMP4_ECHO_REQUEST 8
#define ICMP4_TIME_EXCEEDED 11
#define ICMP4_PARAM_PROB 12
#define ICMP4_TIMESTAMP 13
#define ICMP4_TIMESTAMPREPLY 14
#define ICMP4_INFOREQUEST 15
#define ICMP4_INFOREPLY 16

#define ICMP_HEADER_LEN 8
struct icmp_header {
    uint8_t icmp_type;
    uint8_t icmp_code;
    ovs_be16 icmp_csum;
    union {
        struct {
            ovs_be16 id;
            ovs_be16 seq;
        } echo;
        struct {
            ovs_be16 empty;
            ovs_be16 mtu;
        } frag;
        ovs_16aligned_be32 gateway;
    } icmp_fields;
};
BUILD_ASSERT_DECL(ICMP_HEADER_LEN == sizeof(struct icmp_header));

/* ICMPV4 */
#define ICMP_ERROR_DATA_L4_LEN 8

#define IGMP_HEADER_LEN 8
struct igmp_header {
    uint8_t igmp_type;
    uint8_t igmp_code;
    ovs_be16 igmp_csum;
    ovs_16aligned_be32 group;
};
BUILD_ASSERT_DECL(IGMP_HEADER_LEN == sizeof(struct igmp_header));

#define IGMPV3_HEADER_LEN 8
struct igmpv3_header {
    uint8_t type;
    uint8_t rsvr1;
    ovs_be16 csum;
    ovs_be16 rsvr2;
    ovs_be16 ngrp;
};
BUILD_ASSERT_DECL(IGMPV3_HEADER_LEN == sizeof(struct igmpv3_header));

#define IGMPV3_QUERY_HEADER_LEN 12
struct igmpv3_query_header {
    uint8_t type;
    uint8_t max_resp;
    ovs_be16 csum;
    ovs_16aligned_be32 group;
    uint8_t srs_qrv;
    uint8_t qqic;
    ovs_be16 nsrcs;
};
BUILD_ASSERT_DECL(
    IGMPV3_QUERY_HEADER_LEN == sizeof(struct igmpv3_query_header
));

#define IGMPV3_RECORD_LEN 8
struct igmpv3_record {
    uint8_t type;
    uint8_t aux_len;
    ovs_be16 nsrcs;
    ovs_16aligned_be32 maddr;
};
BUILD_ASSERT_DECL(IGMPV3_RECORD_LEN == sizeof(struct igmpv3_record));

#define IGMP_HOST_MEMBERSHIP_QUERY    0x11 /* From RFC1112 */
#define IGMP_HOST_MEMBERSHIP_REPORT   0x12 /* Ditto */
#define IGMPV2_HOST_MEMBERSHIP_REPORT 0x16 /* V2 version of 0x12 */
#define IGMP_HOST_LEAVE_MESSAGE       0x17
#define IGMPV3_HOST_MEMBERSHIP_REPORT 0x22 /* V3 version of 0x12 */

/*
 * IGMPv3 and MLDv2 use the same codes.
 */
#define IGMPV3_MODE_IS_INCLUDE 1
#define IGMPV3_MODE_IS_EXCLUDE 2
#define IGMPV3_CHANGE_TO_INCLUDE_MODE 3
#define IGMPV3_CHANGE_TO_EXCLUDE_MODE 4
#define IGMPV3_ALLOW_NEW_SOURCES 5
#define IGMPV3_BLOCK_OLD_SOURCES 6

#define SCTP_HEADER_LEN 12
struct sctp_header {
    ovs_be16 sctp_src;
    ovs_be16 sctp_dst;
    ovs_16aligned_be32 sctp_vtag;
    ovs_16aligned_be32 sctp_csum;
};
BUILD_ASSERT_DECL(SCTP_HEADER_LEN == sizeof(struct sctp_header));

#define UDP_HEADER_LEN 8
struct udp_header {
    ovs_be16 udp_src;
    ovs_be16 udp_dst;
    ovs_be16 udp_len;
    ovs_be16 udp_csum;
};
BUILD_ASSERT_DECL(UDP_HEADER_LEN == sizeof(struct udp_header));

#define ESP_HEADER_LEN 8
struct esp_header {
    ovs_be32 spi;
    ovs_be32 seq_no;
};
BUILD_ASSERT_DECL(ESP_HEADER_LEN == sizeof(struct esp_header));

#define ESP_TRAILER_LEN 2
struct esp_trailer {
    uint8_t pad_len;
    uint8_t next_hdr;
};
BUILD_ASSERT_DECL(ESP_TRAILER_LEN == sizeof(struct esp_trailer));

#define TCP_FIN 0x001
#define TCP_SYN 0x002
#define TCP_RST 0x004
#define TCP_PSH 0x008
#define TCP_ACK 0x010
#define TCP_URG 0x020
#define TCP_ECE 0x040
#define TCP_CWR 0x080
#define TCP_NS  0x100

#define TCP_CTL(flags, offset) (htons((flags) | ((offset) << 12)))
#define TCP_FLAGS(tcp_ctl) (ntohs(tcp_ctl) & 0x0fff)
#define TCP_FLAGS_BE16(tcp_ctl) ((tcp_ctl) & htons(0x0fff))
#define TCP_OFFSET(tcp_ctl) (ntohs(tcp_ctl) >> 12)

#define TCP_HEADER_LEN 20
struct tcp_header {
    ovs_be16 tcp_src;
    ovs_be16 tcp_dst;
    ovs_16aligned_be32 tcp_seq;
    ovs_16aligned_be32 tcp_ack;
    ovs_be16 tcp_ctl;
    ovs_be16 tcp_winsz;
    ovs_be16 tcp_csum;
    ovs_be16 tcp_urg;
};
BUILD_ASSERT_DECL(TCP_HEADER_LEN == sizeof(struct tcp_header));

/* Connection states.
 *
 * Names like CS_RELATED are bit values, e.g. 1 << 2.
 * Names like CS_RELATED_BIT are bit indexes, e.g. 2. */
#define CS_STATES                               \
    CS_STATE(NEW,         0, "new")             \
    CS_STATE(ESTABLISHED, 1, "est")             \
    CS_STATE(RELATED,     2, "rel")             \
    CS_STATE(REPLY_DIR,   3, "rpl")             \
    CS_STATE(INVALID,     4, "inv")             \
    CS_STATE(TRACKED,     5, "trk")             \
    CS_STATE(SRC_NAT,     6, "snat")            \
    CS_STATE(DST_NAT,     7, "dnat")

enum {
#define CS_STATE(ENUM, INDEX, NAME) \
    CS_##ENUM = 1 << INDEX, \
    CS_##ENUM##_BIT = INDEX,
    CS_STATES
#undef CS_STATE
};

/* Undefined connection state bits. */
enum {
#define CS_STATE(ENUM, INDEX, NAME) +CS_##ENUM
    CS_SUPPORTED_MASK = CS_STATES
#undef CS_STATE
};
#define CS_UNSUPPORTED_MASK  (~(uint32_t)CS_SUPPORTED_MASK)

#define ARP_HRD_ETHERNET 1
#define ARP_PRO_IP 0x0800
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2
#define ARP_OP_RARP 3

#define ARP_ETH_HEADER_LEN 28
struct arp_eth_header {
    /* Generic members. */
    ovs_be16 ar_hrd;           /* Hardware type. */
    ovs_be16 ar_pro;           /* Protocol type. */
    uint8_t ar_hln;            /* Hardware address length. */
    uint8_t ar_pln;            /* Protocol address length. */
    ovs_be16 ar_op;            /* Opcode. */

    /* Ethernet+IPv4 specific members. */
    struct eth_addr ar_sha;     /* Sender hardware address. */
    ovs_16aligned_be32 ar_spa;  /* Sender protocol address. */
    struct eth_addr ar_tha;     /* Target hardware address. */
    ovs_16aligned_be32 ar_tpa;  /* Target protocol address. */
};
BUILD_ASSERT_DECL(ARP_ETH_HEADER_LEN == sizeof(struct arp_eth_header));




/* Minimum value for an Ethernet type.  Values below this are IEEE 802.2 frame
 * lengths. */
#define ETH_TYPE_MIN           0x600

#define ETH_HEADER_LEN 14
#define ETH_PAYLOAD_MIN 46
#define ETH_PAYLOAD_MAX 1500
#define ETH_TOTAL_MIN (ETH_HEADER_LEN + ETH_PAYLOAD_MIN)
#define ETH_TOTAL_MAX (ETH_HEADER_LEN + ETH_PAYLOAD_MAX)
#define ETH_VLAN_TOTAL_MAX (ETH_HEADER_LEN + VLAN_HEADER_LEN + ETH_PAYLOAD_MAX)
struct eth_header {
    struct eth_addr eth_dst;
    struct eth_addr eth_src;
    ovs_be16 eth_type;
};
BUILD_ASSERT_DECL(ETH_HEADER_LEN == sizeof(struct eth_header));

#if 0 /* liwei */
void push_eth(struct dp_packet *packet, const struct eth_addr *dst,
              const struct eth_addr *src);
void pop_eth(struct dp_packet *packet);

void push_nsh(struct dp_packet *packet, const struct nsh_hdr *nsh_hdr_src);
bool pop_nsh(struct dp_packet *packet);
#endif

#define LLC_DSAP_SNAP 0xaa
#define LLC_SSAP_SNAP 0xaa
#define LLC_CNTL_SNAP 3

#define LLC_HEADER_LEN 3
struct llc_header {
    uint8_t llc_dsap;
    uint8_t llc_ssap;
    uint8_t llc_cntl;
};
BUILD_ASSERT_DECL(LLC_HEADER_LEN == sizeof(struct llc_header));

/* LLC field values used for STP frames. */
#define STP_LLC_SSAP 0x42
#define STP_LLC_DSAP 0x42
#define STP_LLC_CNTL 0x03

#define SNAP_ORG_ETHERNET "\0\0" /* The compiler adds a null byte, so
                                    sizeof(SNAP_ORG_ETHERNET) == 3. */
#define SNAP_HEADER_LEN 5
OVS_PACKED(
struct snap_header {
    uint8_t snap_org[3];
    ovs_be16 snap_type;
});
BUILD_ASSERT_DECL(SNAP_HEADER_LEN == sizeof(struct snap_header));

#define LLC_SNAP_HEADER_LEN (LLC_HEADER_LEN + SNAP_HEADER_LEN)
OVS_PACKED(
struct llc_snap_header {
    struct llc_header llc;
    struct snap_header snap;
});
BUILD_ASSERT_DECL(LLC_SNAP_HEADER_LEN == sizeof(struct llc_snap_header));

#define VLAN_VID_MASK 0x0fff
#define VLAN_VID_SHIFT 0

#define VLAN_PCP_MASK 0xe000
#define VLAN_PCP_SHIFT 13

#define VLAN_CFI 0x1000
#define VLAN_CFI_SHIFT 12

/* Given the vlan_tci field from an 802.1Q header, in network byte order,
 * returns the VLAN ID in host byte order. */
static inline uint16_t
vlan_tci_to_vid(ovs_be16 vlan_tci)
{
    return (ntohs(vlan_tci) & VLAN_VID_MASK) >> VLAN_VID_SHIFT;
}

/* Given the vlan_tci field from an 802.1Q header, in network byte order,
 * returns the priority code point (PCP) in host byte order. */
static inline int
vlan_tci_to_pcp(ovs_be16 vlan_tci)
{
    return (ntohs(vlan_tci) & VLAN_PCP_MASK) >> VLAN_PCP_SHIFT;
}

/* Given the vlan_tci field from an 802.1Q header, in network byte order,
 * returns the Canonical Format Indicator (CFI). */
static inline int
vlan_tci_to_cfi(ovs_be16 vlan_tci)
{
    return (vlan_tci & htons(VLAN_CFI)) != 0;
}

#define VLAN_HEADER_LEN 4
struct vlan_header {
    ovs_be16 vlan_tci;          /* Lowest 12 bits are VLAN ID. */
    ovs_be16 vlan_next_type;
};
BUILD_ASSERT_DECL(VLAN_HEADER_LEN == sizeof(struct vlan_header));

#define VLAN_ETH_HEADER_LEN (ETH_HEADER_LEN + VLAN_HEADER_LEN)
struct vlan_eth_header {
    struct eth_addr veth_dst;
    struct eth_addr veth_src;
    ovs_be16 veth_type;         /* Always htons(ETH_TYPE_VLAN). */
    ovs_be16 veth_tci;          /* Lowest 12 bits are VLAN ID. */
    ovs_be16 veth_next_type;
};
BUILD_ASSERT_DECL(VLAN_ETH_HEADER_LEN == sizeof(struct vlan_eth_header));

bool
ipv6_parse(const char *s, struct in6_addr *ip);

#define IPV6_HEADER_LEN 40

/* Like struct in6_addr, but whereas that struct requires 32-bit alignment on
 * most implementations, this one only requires 16-bit alignment. */
union ovs_16aligned_in6_addr {
    ovs_be16 be16[8];
    ovs_16aligned_be32 be32[4];
};

/* Like struct ip6_hdr, but whereas that struct requires 32-bit alignment, this
 * one only requires 16-bit alignment. */
struct ovs_16aligned_ip6_hdr {
    union {
        struct ovs_16aligned_ip6_hdrctl {
            ovs_16aligned_be32 ip6_un1_flow;
            ovs_be16 ip6_un1_plen;
            uint8_t ip6_un1_nxt;
            uint8_t ip6_un1_hlim;
        } ip6_un1;
        uint8_t ip6_un2_vfc;
    } ip6_ctlun;
    union ovs_16aligned_in6_addr ip6_src;
    union ovs_16aligned_in6_addr ip6_dst;
};

/* Like struct in6_frag, but whereas that struct requires 32-bit alignment,
 * this one only requires 16-bit alignment. */
struct ovs_16aligned_ip6_frag {
    uint8_t ip6f_nxt;
    uint8_t ip6f_reserved;
    ovs_be16 ip6f_offlg;
    ovs_16aligned_be32 ip6f_ident;
};

#define ICMP6_HEADER_LEN 4
struct icmp6_header {
    uint8_t icmp6_type;
    uint8_t icmp6_code;
    ovs_be16 icmp6_cksum;
};
BUILD_ASSERT_DECL(ICMP6_HEADER_LEN == sizeof(struct icmp6_header));

#define ICMP6_DATA_HEADER_LEN 8
struct icmp6_data_header {
    struct icmp6_header icmp6_base;
    union {
        ovs_16aligned_be32 be32[1];
        ovs_be16           be16[2];
        uint8_t            u8[4];
    } icmp6_data;
};
BUILD_ASSERT_DECL(ICMP6_DATA_HEADER_LEN == sizeof(struct icmp6_data_header));



/* The IPv6 flow label is in the lower 20 bits of the first 32-bit word. */
#define IPV6_LABEL_MASK 0x000fffff

/* Example:
 *
 * char *string = "1 ::1 2";
 * char ipv6_s[IPV6_SCAN_LEN + 1];
 * struct in6_addr ipv6;
 *
 * if (ovs_scan(string, "%d"IPV6_SCAN_FMT"%d", &a, ipv6_s, &b)
 *     && inet_pton(AF_INET6, ipv6_s, &ipv6) == 1) {
 *     ...
 * }
 */
#define IPV6_SCAN_FMT "%46[0123456789abcdefABCDEF:.]"
#define IPV6_SCAN_LEN 46

extern const struct in6_addr in6addr_exact;
#define IN6ADDR_EXACT_INIT { { { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, \
                                 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff } } }

extern const struct in6_addr in6addr_all_hosts;
#define IN6ADDR_ALL_HOSTS_INIT { { { 0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00, \
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01 } } }

extern const struct in6_addr in6addr_all_routers;
#define IN6ADDR_ALL_ROUTERS_INIT { { { 0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00, \
                                       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02 } } }

static inline bool ipv6_addr_equals(const struct in6_addr *a,
                                    const struct in6_addr *b)
{
#ifdef IN6_ARE_ADDR_EQUAL
    return IN6_ARE_ADDR_EQUAL(a, b);
#else
    return !memcmp(a, b, sizeof(*a));
#endif
}

/* Checks the IPv6 address in 'mask' for all zeroes. */
static inline bool ipv6_mask_is_any(const struct in6_addr *mask) {
    return ipv6_addr_equals(mask, &in6addr_any);
}

static inline bool ipv6_mask_is_exact(const struct in6_addr *mask) {
    return ipv6_addr_equals(mask, &in6addr_exact);
}

static inline bool ipv6_is_all_hosts(const struct in6_addr *addr) {
    return ipv6_addr_equals(addr, &in6addr_all_hosts);
}

static inline bool ipv6_addr_is_set(const struct in6_addr *addr) {
    return !ipv6_addr_equals(addr, &in6addr_any);
}

static inline bool ipv6_addr_is_multicast(const struct in6_addr *ip) {
    return ip->s6_addr[0] == 0xff;
}

static inline struct in6_addr
in6_addr_mapped_ipv4(ovs_be32 ip4)
{
    struct in6_addr ip6;
    memset(&ip6, 0, sizeof(ip6));
    ip6.s6_addr[10] = 0xff, ip6.s6_addr[11] = 0xff;
    memcpy(&ip6.s6_addr[12], &ip4, 4);
    return ip6;
}

static inline void
in6_addr_set_mapped_ipv4(struct in6_addr *ip6, ovs_be32 ip4)
{
    *ip6 = in6_addr_mapped_ipv4(ip4);
}

static inline ovs_be32
in6_addr_get_mapped_ipv4(const struct in6_addr *addr)
{
    union ovs_16aligned_in6_addr *taddr =
        (union ovs_16aligned_in6_addr *) addr;
    if (IN6_IS_ADDR_V4MAPPED(addr)) {
        return get_16aligned_be32(&taddr->be32[3]);
    } else {
        return INADDR_ANY;
    }
}

void in6_addr_solicited_node(struct in6_addr *addr,
                             const struct in6_addr *ip6);

void in6_generate_eui64(struct eth_addr ea, const struct in6_addr *prefix,
                        struct in6_addr *lla);

void in6_generate_lla(struct eth_addr ea, struct in6_addr *lla);

/* Returns true if 'addr' is a link local address.  Otherwise, false. */
bool in6_is_lla(struct in6_addr *addr);

void ipv6_multicast_to_ethernet(struct eth_addr *eth,
                                const struct in6_addr *ip6);

static inline bool dl_type_is_ip_any(ovs_be16 dl_type)
{
    return dl_type == htons(ETH_TYPE_IP)
        || dl_type == htons(ETH_TYPE_IPV6);
}

size_t strlcat(char *__restrict dest,
	       const char *__restrict src, size_t destsize);

size_t strlcpy(char *__restrict dest,
	       const char *__restrict src, size_t destsize);

/* Flag manipulation macros. */
#define CHECK_FLAG(V,F)      ((V) & (F))
#define SET_FLAG(V,F)        (V) |= (F)
#define UNSET_FLAG(V,F)      (V) &= ~(F)
#define RESET_FLAG(V)        (V) = 0
#define COND_FLAG(V, F, C)   ((C) ? (SET_FLAG(V, F)) : (UNSET_FLAG(V, F)))

/* Atomic flag manipulation macros. */
#define CHECK_FLAG_ATOMIC(PV, F)                                               \
    ((atomic_load_explicit(PV, memory_order_seq_cst)) & (F))
#define SET_FLAG_ATOMIC(PV, F)                                                 \
    ((atomic_fetch_or_explicit(PV, (F), memory_order_seq_cst)))
#define UNSET_FLAG_ATOMIC(PV, F)                                               \
    ((atomic_fetch_and_explicit(PV, ~(F), memory_order_seq_cst)))
#define RESET_FLAG_ATOMIC(PV)                                                  \
    ((atomic_store_explicit(PV, 0, memory_order_seq_cst)))

/* Address family numbers from RFC1700. */
typedef enum {
	AFI_UNSPEC = 0,
	AFI_IP = 1,
	AFI_IP6 = 2,
	AFI_L2VPN = 3,
	AFI_MAX = 4
} afi_t;

#define IS_VALID_AFI(a) ((a) > AFI_UNSPEC && (a) < AFI_MAX)

/* Subsequent Address Family Identifier. */
typedef enum {
	SAFI_UNSPEC = 0,
	SAFI_UNICAST = 1,
	SAFI_MULTICAST = 2,
	SAFI_MPLS_VPN = 3,
	SAFI_ENCAP = 4,
	SAFI_EVPN = 5,
	SAFI_LABELED_UNICAST = 6,
	SAFI_FLOWSPEC = 7,
	SAFI_MAX = 8
} safi_t;

#define FOREACH_AFI_SAFI(afi, safi)                                            \
	for (afi = AFI_IP; afi < AFI_MAX; afi++)                               \
		for (safi = SAFI_UNICAST; safi < SAFI_MAX; safi++)

#define FOREACH_AFI_SAFI_NSF(afi, safi)                                        \
	for (afi = AFI_IP; afi < AFI_MAX; afi++)                               \
		for (safi = SAFI_UNICAST; safi <= SAFI_MPLS_VPN; safi++)


typedef int32_t ifindex_t;

/* Both readn and writen are deprecated and will be removed.  They are not
   suitable for use with non-blocking file descriptors.
 */
extern int readn(int, uint8_t *, int);
extern int writen(int, const uint8_t *, int);

/* Set the file descriptor to use non-blocking I/O.  Returns 0 for success,
   -1 on error. */
extern int set_nonblocking(int fd);

extern int set_cloexec(int fd);

/* Does the I/O error indicate that the operation should be retried later? */
#define ERRNO_IO_RETRY(EN)                                                     \
	(((EN) == EAGAIN) || ((EN) == EWOULDBLOCK) || ((EN) == EINTR))

extern float htonf(float);
extern float ntohf(float);

typedef uint32_t sock_handle_t;

#define  ETHER_ADDR_LEN (6)

/* Socket address structure for L2 */
struct sockaddr_l2
{
  /* Destination Mac address */
  unsigned char dest_mac[ETHER_ADDR_LEN];

  /* Source Mac address */
  unsigned char src_mac[ETHER_ADDR_LEN];

  /* Outgoing/Incoming interface index */
  unsigned int port;
};

pid_t get_pid_by_name(const char *process_name);

#endif /* __HAVE_UTIL_H__ */
