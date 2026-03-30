#ifndef __HAVE_COMPILER_H__
#define __HAVE_COMPILER_H__

#include <stddef.h>
#include <stdbool.h>

/* Are two types/vars the same type (ignoring qualifiers)? */
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define __alias(symbol)                 __attribute__((__alias__(#symbol)))
#define __aligned(x)                    __attribute__((__aligned__(x)))
#define __aligned_largest               __attribute__((__aligned__))
#if 0
#define __alloc_size__(x, ...)		    __attribute__((__alloc_size__(x, ## __VA_ARGS__)))
#endif
#ifndef __always_inline
#define __always_inline                 inline __attribute__((__always_inline__))
#endif /* __always_inline__ */
#define __cold                          __attribute__((__cold__))
#ifndef __attribute_const__
#define __attribute_const__             __attribute__((__const__))
#endif /* __attribute_const__ */
#define __copy(symbol)                 __attribute__((__copy__(symbol)))
#define __designated_init              __attribute__((__designated_init__))
#define __visible                      __attribute__((__externally_visible__))
#define __compiletime_error(message) __attribute__((error(message)))
#define __printf(a, b)                  __attribute__((__format__(printf, a, b)))
#define __scanf(a, b)                   __attribute__((__format__(scanf, a, b)))
#define __gnu_inline                    __attribute__((__gnu_inline__))
#define __malloc                        __attribute__((__malloc__))
#define __mode(x)                       __attribute__((__mode__(x)))
#define __noclone                      __attribute__((__noclone__))
#define fallthrough                    __attribute__((__fallthrough__))
#define __flatten			__attribute__((flatten))
#define __noinline                      __attribute__((__noinline__))
#define __nonstring                    __attribute__((__nonstring__))
#define __no_profile                  __attribute__((__no_profile_instrument_function__))
#define __noreturn                      __attribute__((__noreturn__))
#define __packed                        __attribute__((__packed__))
#define __section(section)              __attribute__((__section__(section)))
#define __always_unused                 __attribute__((__unused__))
#define __maybe_unused                  __attribute__((__unused__))
#define __used                          __attribute__((__used__))
#define __must_check                    __attribute__((__warn_unused_result__))
#define __compiletime_warning(msg)     __attribute__((__warning__(msg)))
#define __disable_sanitizer_instrumentation \
	 __attribute__((disable_sanitizer_instrumentation))
#define __weak                          __attribute__((__weak__))

#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)

#define QUOTE(name) #name
#define STR(value) QUOTE(value)

/* OVS_NO_RETURN: 
 * Informs the compiler that the function does not return. 
 * The compiler can then perform optimizations by removing code that is never reached.
 * Use this attribute to reduce the cost of calling a function that never returns, 
 * such as exit().
 * Best practice is to always terminate non-returning functions with while(1);.
 */
#define OVS_NO_RETURN __attribute__((__noreturn__))

/* OVS_UNUSED: 
 * The unused function attribute prevents the compiler from generating
 * warnings if the function is not referenced. This does not change the 
 * behavior of the unused function removal process.
 */
#define OVS_UNUSED __attribute__((__unused__))

/* OVS_PRINTF_FORMAT:
 * format (archetype, string-index, first-to-check)
 * The format attribute specifies that a function takes printf or scanf style arguments which should be type-checked against a format string. For example, the following declaration causes the compiler to check the arguments in calls to my_ printf for consistency with the printf style format string argument, my_format.
 * extern int 
 * my_printf (void *my_object, const char *my_format, ...) 
 *          __attribute__ ((format (printf, 2, 3)));
 *   The parameter, "archetype", determines how the format string is interpreted, and should be either printf, scanf or strftime.
 *   The parameter, "string-index", specifies which argument is the format string argument (starting from 1), while "first-to-check" is the number of the first argument to check against the format string. For functions where the arguments are not available to be checked (such as vprintf), specify the third parameter as zero. In this case the compiler only checks the format string for consistency.
 *   In the previous example, the format string (my_format) is the second argument of the function my_print, and the arguments to check start with the third argument, so the correct parameters for the format attribute are 2 and 3.
 *   The format attribute allows you to identify your own functions which take format strings as arguments, so that GNU CC can check the calls to these functions for errors. The compiler always checks formats for the ANSI library functions printf, fprintf, sprintf, scanf, fscanf, sscanf, strftime, vprintf, vfprintf and vsprintf whenever such warnings are requested (using ‘-Wformat’), so there is no need to modify the header file, ‘stdio.h’.
 */
#define OVS_PRINTF_FORMAT(FMT, ARG1) __attribute__((__format__(printf, FMT, ARG1)))
#define OVS_SCANF_FORMAT(FMT, ARG1) __attribute__((__format__(scanf, FMT, ARG1)))
/* The warn_unused_result attribute causes a warning to be emitted if a caller of the function with this attribute does not use its return value. This is useful for functions where not checking the result is either a security problem or always a bug, such as realloc.
   int fn () __attribute__ ((warn_unused_result));
   int foo ()
   {
     if (fn () < 0) return -1;
     fn ();
     return 0;
   }
results in warning on line 5.
 *
 */
#define OVS_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
/* OVS_LIKELY/OVS_UNLIKELY:
 * long __builtin_expect (long EXP, long C)
     You may use `__builtin_expect' to provide the compiler with branch
     prediction information.  In general, you should prefer to use
     actual profile feedback for this (`-fprofile-arcs'), as
     programmers are notoriously bad at predicting how their programs
     actually perform.  However, there are applications in which this
     data is hard to collect.

     The return value is the value of EXP, which should be an integral
     expression.  The value of C must be a compile-time constant.  The
     semantics of the built-in are that it is expected that EXP == C.
     For example:

          if (__builtin_expect (x, 0))
            foo ();

     would indicate that we do not expect to call `foo', since we
     expect `x' to be zero.  Since you are limited to integral
     expressions for EXP, you should use constructions such as

          if (__builtin_expect (ptr != NULL, 1))
            error ();

     when testing pointer or floating-point values.

It optimizes things by ordering the generated assembly code correctly, to optimize the usage of the processor pipeline. 
To do so, they arrange the code so that the likeliest branch is executed without performing any jmp instruction 
(which has the bad effect of flushing the processor pipeline).
*/
#define OVS_LIKELY(CONDITION) __builtin_expect(!!(CONDITION), 1)
#define OVS_UNLIKELY(CONDITION) __builtin_expect(!!(CONDITION), 0)

/* packed/align
 * Data structure alignment is the way data is arranged and accessed in computer memory. It consists of two separate but related issues: data alignment and data structure padding.

When a modern computer reads from or writes to a memory address, it will do this in word sized chunks (e.g. 4 byte chunks on a 32-bit system). Data alignment means putting the data at a memory offset equal to some multiple of the word size, which increases the system's performance due to the way the CPU handles memory.

To align the data, it may be necessary to insert some meaningless bytes between the end of the last data structure and the start of the next, which is data structure padding.

gcc provides functionality to disable structure padding. i.e to avoid these meaningless bytes in some cases. Consider the following structure:

typedef struct
{
     char Data1;
     int Data2;
     unsigned short Data3;
     char Data4;

}sSampleStruct;
sizeof(sSampleStruct) will be 12 rather than 8. Because of structure padding. By default, In X86, structures will be padded to 4-byte alignment:

typedef struct
{
     char Data1;
     //3-Bytes Added here.
     int Data2;
     unsigned short Data3;
     char Data4;
     //1-byte Added here.

}sSampleStruct;
We can use __attribute__((packed, aligned(X))) to insist particular(X) sized padding. X should be powers of two. Refer here

typedef struct
{
     char Data1;
     int Data2;
     unsigned short Data3;
     char Data4;

}__attribute__((packed, aligned(1))) sSampleStruct;  
so the above specified gcc attribute does not allow the structure padding. so the size will be 8 bytes.

If you wish to do the same for all the structures, simply we can push the alignment value to stack using #pragma

#pragma pack(push, 1)

//Structure 1
......

//Structure 2
......

#pragma pack(pop)
 */
#define OVS_PACKED_ENUM __attribute__((__packed__))
#define OVS_PACKED(DECL) DECL __attribute__((__packed__))
#define OVS_ALIGNED_STRUCT(N, TAG) struct __attribute__((aligned(N))) TAG
#define OVS_ALIGNED_VAR(N) __attribute__((aligned(N)))

/* Supplies code to be run at startup time before invoking main().
 * Use as:
 *
 *     OVS_CONSTRUCTOR(my_constructor) {
 *         ...some code...
 *     }
 */
#define OVS_CONSTRUCTOR(f) \
    static void f(void) __attribute__((constructor)); \
    static void f(void)

/* OVS_PREFETCH() can be used to instruct the CPU to fetch the cache
 * line containing the given address to a CPU cache.
 * OVS_PREFETCH_WRITE() should be used when the memory is going to be
 * written to.  Depending on the target CPU, this can generate the same
 * instruction as OVS_PREFETCH(), or bring the data into the cache in an
 * exclusive state. */
#define OVS_PREFETCH(addr) __builtin_prefetch((addr))
#define OVS_PREFETCH_WRITE(addr) __builtin_prefetch((addr), 1)

/* Compile time assertion:
 * 注意kernel这个宏的实现:
 * #define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
 * When condition is true, this becomes ((void)sizeof(char[-1])), which is illegal and should fail at compile time, and otherwise it becomes ((void)sizeof(char[1])), which is just fine.
 * 然后用这个逻辑去分析BUILD_ASSERT的实现。
 * 注意build_assert_failed的类型是unsigned int，如果将-1赋给这这个变量的话，编译器会检测到这个错误。
 */

/*
 * Force a compilation error if condition is true, but also produce a
 * result (of value 0 and type int), so the expression can be used
 * e.g. in a structure initializer (or where-ever else comma expressions
 * aren't permitted).
 */
#define BUILD_BUG_ON_ZERO(e) ((int)(sizeof(struct { int:(-!!(e)); })))

/* Force a compilation error if a constant expression is not a power of 2 */
#define __BUILD_BUG_ON_NOT_POWER_OF_2(n)    \
    BUILD_BUG_ON(((n) & ((n) - 1)) != 0)
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)          \
    BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))

/*
 * BUILD_BUG_ON_INVALID() permits the compiler to check the validity of the
 * expression but avoids the generation of any code, even if that expression
 * has side-effects.
 */
#define BUILD_BUG_ON_INVALID(e) ((void)(sizeof((__force long)(e))))

/**
 * BUILD_BUG_ON_MSG - break compile if a condition is true & emit supplied
 *            error message.
 * @condition: the condition which the compiler should know is false.
 *
 * See BUILD_BUG_ON for description.
 */

#define __compiletime_assert(condition, msg, prefix, suffix)       \
    do {                                \
        /*                          \
         * __noreturn is needed to give the compiler enough \
         * information to avoid certain possibly-uninitialized  \
         * warnings (regardless of the build failing).      \
         */                         \
        __noreturn extern void prefix ## suffix(void)       \
            __compiletime_error(msg);           \
        if (!(condition))                   \
            prefix ## suffix();             \
    } while (0)

#define _compiletime_assert(condition, msg, prefix, suffix) \
    __compiletime_assert(condition, msg, prefix, suffix)

/**
 * compiletime_assert - break build and emit msg if condition is false
 * @condition: a compile-time constant condition to check
 * @msg:       a message to emit if condition is false
 *
 * In tradition of POSIX assert, this macro will break the build if the
 * supplied condition is *false*, emitting the supplied error message if the
 * compiler has support to do so.
 */
#define compiletime_assert(condition, msg) \
    _compiletime_assert(condition, msg, __compiletime_assert_, __COUNTER__)

#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)

/**
 * BUILD_BUG_ON - break compile if a condition is true.
 * @condition: the condition which the compiler should know is false.
 *
 * If you have some code which relies on certain constants being equal, or
 * some other compile-time-evaluated condition, you should use BUILD_BUG_ON to
 * detect if someone changes it.
 */

#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)

/**
 * BUILD_BUG - break compile if used.
 *
 * If you have some code that you expect the compiler to eliminate at
 * build time, you should use BUILD_BUG to detect if it is
 * unexpectedly used.
 */
#define BUILD_BUG() BUILD_BUG_ON_MSG(1, "BUILD_BUG failed")

/**
 * static_assert - check integer constant expression at build time
 *
 * static_assert() is a wrapper for the C11 _Static_assert, with a
 * little macro magic to make the message optional (defaulting to the
 * stringification of the tested expression).
 *
 * Contrary to BUILD_BUG_ON(), static_assert() can be used at global
 * scope, but requires the expression to be an integer constant
 * expression (i.e., it is not enough that __builtin_constant_p() is
 * true for expr).
 *
 * Also note that BUILD_BUG_ON() fails the build if the condition is
 * true, while static_assert() fails the build if the expression is
 * false.
 */
#ifndef static_assert
#define static_assert(expr, ...) __static_assert(expr, ##__VA_ARGS__, #expr)
#define __static_assert(expr, msg, ...) _Static_assert(expr, msg)
#endif /* static_assert */

/* Build assertions.
 *
 * Use BUILD_ASSERT_DECL as a declaration or a statement, or BUILD_ASSERT as
 * part of an expression. */
#define BUILD_ASSERT__(EXPR) \
        sizeof(struct { unsigned int build_assert_failed : (EXPR) ? 1 : -1; })
#define BUILD_ASSERT(EXPR) (void) BUILD_ASSERT__(EXPR)
#define BUILD_ASSERT_DECL(EXPR) \
        extern int (*build_assert(void))[BUILD_ASSERT__(EXPR)]

#define BUILD_ASSERT_GCCONLY(EXPR) BUILD_ASSERT(EXPR)
#define BUILD_ASSERT_DECL_GCCONLY(EXPR) BUILD_ASSERT_DECL(EXPR)

#define STRFTIME_FORMAT(FMT) __attribute__((__format__(__strftime__, FMT, 0)))
/* malloc
 * The malloc attribute is used to tell the compiler that a function may be treated as if any non-NULL pointer it returns cannot alias any other pointer valid when the function returns and that the memory has undefined content. This will often improve optimization. Standard functions with this property include malloc and calloc. realloc-like functions do not have this property as the memory pointed to does not have undefined content.
 */
#define MALLOC_LIKE __attribute__((__malloc__))
#define ALWAYS_INLINE __attribute__((always_inline))
/* sentinel
 * This function attribute ensures that a parameter in a function call is an explicit NULL. 
 * The attribute is only valid on variadic functions. By default, the sentinel is located at position zero, the last parameter of the function call. If an optional integer position argument P is supplied to the attribute, the sentinel must be located at position P counting backwards from the end of the argument list.
 *  __attribute__ ((sentinel))
 *    is equivalent to
 *  __attribute__ ((sentinel(0)))
 *                          
 *The attribute is automatically set with a position of 0 for the built-in functions execl and execlp. The built-in function execle has the attribute set with a position of 1.
 *
 *A valid NULL in this context is defined as zero with any pointer type. If your system defines the NULL macro with an integer type then you need to add an explicit cast. GCC replaces stddef.h with a copy that redefines NULL appropriately.
 *
 *The warnings for missing or incorrect sentinels are enabled with -Wformat.
 */
#define SENTINEL(N) __attribute__((sentinel(N)))

/* _Pragma and pragma
 * The ‘#pragma’ directive is the method specified by the C standard for providing additional information to the compiler, beyond what is conveyed in the language itself. The forms of this directive (commonly known as pragmas) specified by C standard are prefixed with STDC. A C compiler is free to attach any meaning it likes to other pragmas. Most GNU-defined, supported pragmas have been given a GCC prefix.
 *
 * C99 introduced the _Pragma operator. This feature addresses a major problem with ‘#pragma’: being a directive, it cannot be produced as the result of macro expansion. _Pragma is an operator, much like sizeof or defined, and can be embedded in a macro.
 *
 * Its syntax is _Pragma (string-literal), where string-literal can be either a normal or wide-character string literal. It is destringized, by replacing all ‘\\’ with a single ‘\’ and all ‘\"’ with a ‘"’. The result is then processed as if it had appeared as the right hand side of a ‘#pragma’ directive. For example,
 *
 * _Pragma ("GCC dependency \"parse.y\"")
 * has the same effect as #pragma GCC dependency "parse.y". The same effect could be achieved using macros, for example
 *
 * #define DO_PRAGMA(x) _Pragma (#x)
 * DO_PRAGMA (GCC dependency "parse.y")
 *
 * #pragma message string
 * Prints string as a compiler message on compilation. The message is informational only, and is neither a compilation warning nor an error. Newlines can be included in the string by using the ‘\n’ escape sequence.
 *
 * #pragma message "Compiling " __FILE__ "..."
 * string may be parenthesized, and is printed with location information. For example,
 *
 * #define DO_PRAGMA(x) _Pragma (#x)
 * #define TODO(x) DO_PRAGMA(message ("TODO - " #x))
 *
 * TODO(Remember to fix this)
 * prints ‘/tmp/file.c:4: note: #pragma message: TODO - Remember to fix this’.
 */
#define DO_PRAGMA(x) _Pragma(#x)
#define BUILD_MESSAGE(x) \
    DO_PRAGMA(message(x))

# define likely(x)  __builtin_expect(!!(x), 1)
# define unlikely(x)    __builtin_expect(!!(x), 0)
# define likely_notrace(x)  likely(x)
# define unlikely_notrace(x)    unlikely(x)

#ifndef barrier
/* The "volatile" is due to gcc bugs */
# define barrier() __asm__ __volatile__("": : :"memory")
#endif


#define __must_be_array(a)  BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))

#define _RET_NONNULL    , returns_nonnull
/* constructor
 * destructor
 * The constructor attribute causes the function to be called automatically before execution enters main(). Similarly, the destructor attribute causes the function to be called automatically after main() has completed or exit() has been called. Functions with these attributes are useful for initializing data that will be used implicitly during the execution of the program. These attributes are not currently implemented for Objective C.
 */
#define _CONSTRUCTOR(x) constructor(x)
#define _DESTRUCTOR(x)  destructor(x)
#define _ALLOC_SIZE(x)  alloc_size(x)
#define _DEPRECATED(x) deprecated(x)
#define assume(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#define _FALLTHROUGH __attribute__((fallthrough));
#define _OPTIMIZE_O3 __attribute__((optimize("3")))
#define _OPTIMIZE_HOT __attribute__((hot))
#define DSO_PUBLIC __attribute__ ((visibility ("default")))
#define DSO_SELF   __attribute__ ((visibility ("protected")))
#define DSO_LOCAL  __attribute__ ((visibility ("hidden")))

#define OVS_LOCKABLE
#define OVS_REQ_RDLOCK(...)
#define OVS_ACQ_RDLOCK(...)
#define OVS_REQ_WRLOCK(...)
#define OVS_ACQ_WRLOCK(...)
#define OVS_REQUIRES(...)
#define OVS_ACQUIRES(...)
#define OVS_TRY_WRLOCK(...)
#define OVS_TRY_RDLOCK(...)
#define OVS_TRY_LOCK(...)
#define OVS_GUARDED
#define OVS_GUARDED_BY(...)
#define OVS_EXCLUDED(...)
#define OVS_RELEASES(...)
#define OVS_ACQ_BEFORE(...)
#define OVS_ACQ_AFTER(...)
#define OVS_NO_THREAD_SAFETY_ANALYSIS

/* for helper functions defined inside macros */
#define macro_inline	static inline __attribute__((unused))
#define macro_pure	static inline __attribute__((unused, pure))

/* if the macro ends with a function definition */
#define MACRO_REQUIRE_SEMICOLON() \
	_Static_assert(1, "please add a semicolon after this macro")

/* variadic macros, use like:
 * #define V_0()  ...
 * #define V_1(x) ...
 * #define V(...) MACRO_VARIANT(V, ##__VA_ARGS__)(__VA_ARGS__)
 */

/*
 * The reason why we cannot directly define _CONCAT(a, b) as a ## b is because the ## operator is a token-pasting operator that is processed by the preprocessor at compile time, whereas the macro arguments a and b are not processed until the macro is expanded.
 *
 * When the preprocessor encounters a macro definition, it performs a simple text substitution of the macro name with its definition. Therefore, if we define _CONCAT(a, b) as a ## b, the preprocessor would try to concatenate the literal strings "a" and "b" instead of the values of the a and b macro arguments.
 *
 * To make use of the ## operator to concatenate the values of the a and b macro arguments, we need to use an intermediate macro like _CONCAT2 to concatenate the macro arguments into a single token before using the ## operator. This allows us to concatenate the values of a and b rather than their literal string representations.
 *
 * Therefore, we need to define _CONCAT2 as a ## b and then define _CONCAT as _CONCAT2(a, b) in order to concatenate the macro arguments into a single token before using the ## operator.
 */

#define _CONCAT2(a, b) a ## b
#define _CONCAT(a, b) _CONCAT2(a,b)
#define NAMECTR(name) _CONCAT(name, __COUNTER__)

#define _MACRO_VARIANT(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10, N, ...) N
#define MACRO_VARIANT(NAME, ...) \
	_CONCAT(NAME, _MACRO_VARIANT(0, ##__VA_ARGS__, \
			_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0))


/* per-arg repeat macros, use like:
 * #define PERARG(n) ...n...
 * #define FOO(...) MACRO_REPEAT(PERARG, ##__VA_ARGS__)
 */

#define _MACRO_REPEAT_0(NAME)
#define _MACRO_REPEAT_1(NAME, A1) \
	NAME(A1)
#define _MACRO_REPEAT_2(NAME, A1, A2) \
	NAME(A1) NAME(A2)
#define _MACRO_REPEAT_3(NAME, A1, A2, A3) \
	NAME(A1) NAME(A2) NAME(A3)
#define _MACRO_REPEAT_4(NAME, A1, A2, A3, A4) \
	NAME(A1) NAME(A2) NAME(A3) NAME(A4)
#define _MACRO_REPEAT_5(NAME, A1, A2, A3, A4, A5) \
	NAME(A1) NAME(A2) NAME(A3) NAME(A4) NAME(A5)
#define _MACRO_REPEAT_6(NAME, A1, A2, A3, A4, A5, A6) \
	NAME(A1) NAME(A2) NAME(A3) NAME(A4) NAME(A5) NAME(A6)
#define _MACRO_REPEAT_7(NAME, A1, A2, A3, A4, A5, A6, A7) \
	NAME(A1) NAME(A2) NAME(A3) NAME(A4) NAME(A5) NAME(A6) NAME(A7)
#define _MACRO_REPEAT_8(NAME, A1, A2, A3, A4, A5, A6, A7, A8) \
	NAME(A1) NAME(A2) NAME(A3) NAME(A4) NAME(A5) NAME(A6) NAME(A7) NAME(A8)

#define MACRO_REPEAT(NAME, ...) \
	MACRO_VARIANT(_MACRO_REPEAT, ##__VA_ARGS__)(NAME, ##__VA_ARGS__)

/* per-arglist repeat macro, use like this:
 * #define foo(...) MAP_LISTS(F, ##__VA_ARGS__)
 * where F is a n-ary function where n is the number of args in each arglist.
 * e.g.: MAP_LISTS(f, (a, b), (c, d))
 * expands to: f(a, b); f(c, d)
 */


/* Some insane macros to count number of varargs to a functionlike macro */
/* Some test case
 * PP_NARG(A) -> 1
 * PP_NARG(A,B) -> 2
 * PP_NARG(A,B,C) -> 3
 * PP_NARG(A,B,C,D) -> 4
 * PP_NARG(A,B,C,D,E) -> 5
 * PP_NARG(1,2,3,4,5,6,7,8,9,0,
 *          1,2,3,4,5,6,7,8,9,0,
 *          1,2,3,4,5,6,7,8,9,0,
 *          1,2,3,4,5,6,7,8,9,0,
 *          1,2,3,4,5,6,7,8,9,0,
 *          1,2,3,4,5,6,7,8,9,0,
 *          1,2,3) -> 63
 */
#define PP_ARG_N( \
          _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
         _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
         _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
         _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
         _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
         _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
         _61, _62, _63, N, ...) N

#define PP_RSEQ_N()                                        \
         62, 61, 60,                                       \
         59, 58, 57, 56, 55, 54, 53, 52, 51, 50,           \
         49, 48, 47, 46, 45, 44, 43, 42, 41, 40,           \
         39, 38, 37, 36, 35, 34, 33, 32, 31, 30,           \
         29, 28, 27, 26, 25, 24, 23, 22, 21, 20,           \
         19, 18, 17, 16, 15, 14, 13, 12, 11, 10,           \
          9,  8,  7,  6,  5,  4,  3,  2,  1,  0

#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_NARG(...)     PP_NARG_(_, ##__VA_ARGS__, PP_RSEQ_N())
#define ESC(...) __VA_ARGS__

#define MAP_LISTS(M, ...)                                                      \
	_CONCAT(_MAP_LISTS_, PP_NARG(__VA_ARGS__))(M, ##__VA_ARGS__)
#define _MAP_LISTS_0(M)
#define _MAP_LISTS_1(M, _1) ESC(M _1)
#define _MAP_LISTS_2(M, _1, _2) ESC(M _1; M _2)
#define _MAP_LISTS_3(M, _1, _2, _3) ESC(M _1; M _2; M _3)
#define _MAP_LISTS_4(M, _1, _2, _3, _4) ESC(M _1; M _2; M _3; M _4)
#define _MAP_LISTS_5(M, _1, _2, _3, _4, _5) ESC(M _1; M _2; M _3; M _4; M _5)
#define _MAP_LISTS_6(M, _1, _2, _3, _4, _5, _6)                                \
	ESC(M _1; M _2; M _3; M _4; M _5; M _6)
#define _MAP_LISTS_7(M, _1, _2, _3, _4, _5, _6, _7)                            \
	ESC(M _1; M _2; M _3; M _4; M _5; M _6; M _7)
#define _MAP_LISTS_8(M, _1, _2, _3, _4, _5, _6, _7, _8)                        \
	ESC(M _1; M _2; M _3; M _4; M _5; M _6; M _7; M _8)

/* MAX / MIN are not commonly defined, but useful */
/* note: glibc sys/param.h has #define MIN(a,b) (((a)<(b))?(a):(b)) */
#ifdef MAX
#undef MAX
#endif
#define MAX(a, b)                                                              \
	({                                                                     \
		typeof(a) _max_a = (a);                                        \
		typeof(b) _max_b = (b);                                        \
		_max_a > _max_b ? _max_a : _max_b;                             \
	})
#ifdef MIN
#undef MIN
#endif
#define MIN(a, b)                                                              \
	({                                                                     \
		typeof(a) _min_a = (a);                                        \
		typeof(b) _min_b = (b);                                        \
		_min_a < _min_b ? _min_a : _min_b;                             \
	})

#define numcmp(a, b)                                                           \
	({                                                                     \
		typeof(a) _cmp_a = (a);                                        \
		typeof(b) _cmp_b = (b);                                        \
		(_cmp_a < _cmp_b) ? -1 : ((_cmp_a > _cmp_b) ? 1 : 0);          \
	})


#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member)                                \
		({                                                             \
			const typeof(((type *)0)->member) *__mptr = (ptr);         \
			(type *)((char *)__mptr - offsetof(type, member));         \
		})

#define container_of_null(ptr, type, member)                                   \
	({                                                                     \
		typeof(ptr) _tmp = (ptr);                                      \
		_tmp ? container_of(_tmp, type, member) : NULL;                \
	})

#define array_size(ar) (sizeof(ar) / sizeof(ar[0]))

#define static_cast(l, r) (r)
#endif /* __HAVE_COMPILER_H__ */
