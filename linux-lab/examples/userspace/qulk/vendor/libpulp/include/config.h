/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

/* afl support is enabled. */
/* #undef ENABLE_AFL */

/* Disable stack checking routines */
#define ENABLE_STACK_CHECK 0

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "libpulp"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "noreply@suse.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libpulp"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libpulp 0.3.5"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libpulp"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.3.5"

/* Padding nops before the entry point of functions */
#define PRE_NOPS_LEN 14

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Total number of padding nops */
#define ULP_NOPS_LEN 16

/* Total number of padding nops when endbr64 is issued */
#define ULP_NOPS_LEN_ENDBR64 20

/* Version number of package */
#define VERSION "0.3.5"
