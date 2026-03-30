#ifndef __HAVE_UNICODE_H__
#define __HAVE_UNICODE_H__

#include <stdbool.h>
#include <stddef.h>
#include "compiler.h"

/* Returns true if 'c' is a Unicode code point, otherwise false. */
static inline bool
uc_is_code_point(int c)
{
    return c >= 0 && c <= 0x10ffff;
}

/* Returns true if 'c' is a Unicode code point for a leading surrogate. */
static inline bool
uc_is_leading_surrogate(int c)
{
    return c >= 0xd800 && c <= 0xdbff;
}

/* Returns true if 'c' is a Unicode code point for a trailing surrogate. */
static inline bool
uc_is_trailing_surrogate(int c)
{
    return c >= 0xdc00 && c <= 0xdfff;
}

/* Returns true if 'c' is a Unicode code point for a leading or trailing
 * surrogate. */
static inline bool
uc_is_surrogate(int c)
{
    return c >= 0xd800 && c <= 0xdfff;
}

int utf16_decode_surrogate_pair(int leading, int trailing);

size_t utf8_length(const char *);
char *utf8_validate(const char *, size_t *lengthp) OVS_WARN_UNUSED_RESULT;

#endif /* __HAVE_UNICODE_H__ */
