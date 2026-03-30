#ifndef __HAVE_UUID_H__
#define __HAVE_UUID_H__

#include "util.h"

#define UUID_BIT 128            /* Number of bits in a UUID. */
#define UUID_OCTET (UUID_BIT / 8) /* Number of bytes in a UUID. */

/* A Universally Unique IDentifier (UUID) compliant with RFC 4122.
 *
 * Each of the parts is stored in host byte order, but the parts themselves are
 * ordered from left to right.  That is, (parts[0] >> 24) is the first 8 bits
 * of the UUID when output in the standard form, and (parts[3] & 0xff) is the
 * final 8 bits. */
struct uuid {
    uint32_t parts[4];
};
BUILD_ASSERT_DECL(sizeof(struct uuid) == UUID_OCTET);

/* An initializer or expression for an all-zero UUID. */
#define UUID_ZERO ((struct uuid) { .parts = { 0, 0, 0, 0 } })

/* Formats a UUID as a string, in the conventional format.
 *
 * Example:
 *   struct uuid uuid = ...;
 *   printf("This UUID is "UUID_FMT"\n", UUID_ARGS(&uuid));
 *
 */
#define UUID_LEN 36
#define UUID_FMT "%08x-%04x-%04x-%04x-%04x%08x"
#define UUID_ARGS(UUID)                             \
    ((unsigned int) ((UUID)->parts[0])),            \
    ((unsigned int) ((UUID)->parts[1] >> 16)),      \
    ((unsigned int) ((UUID)->parts[1] & 0xffff)),   \
    ((unsigned int) ((UUID)->parts[2] >> 16)),      \
    ((unsigned int) ((UUID)->parts[2] & 0xffff)),   \
    ((unsigned int) ((UUID)->parts[3]))

/* Returns a hash value for 'uuid'.  This hash value is the same regardless of
 * whether we are running on a 32-bit or 64-bit or big-endian or little-endian
 * architecture. */
static inline size_t
uuid_hash(const struct uuid *uuid)
{
    return uuid->parts[0];
}

/* Returns true if 'a == b', false otherwise. */
static inline bool
uuid_equals(const struct uuid *a, const struct uuid *b)
{
    return (a->parts[0] == b->parts[0]
            && a->parts[1] == b->parts[1]
            && a->parts[2] == b->parts[2]
            && a->parts[3] == b->parts[3]);
}

/* Returns the first 'n' hex digits of 'uuid', for 0 < 'n' <= 8.
 *
 * This is useful for displaying a few leading digits of the uuid, e.g. to
 * display 4 digits:
 *     printf("%04x", uuid_prefix(uuid, 4));
 */
static inline unsigned int
uuid_prefix(const struct uuid *uuid, int digits)
{
    return (uuid->parts[0] >> (32 - 4 * digits));
}

void uuid_init(void);
void uuid_generate(struct uuid *);
struct uuid uuid_random(void);
void uuid_zero(struct uuid *);
bool uuid_is_zero(const struct uuid *);
int uuid_compare_3way(const struct uuid *, const struct uuid *);
bool uuid_from_string(struct uuid *, const char *);
bool uuid_from_string_prefix(struct uuid *, const char *);
int uuid_is_partial_string(const char *);
int uuid_is_partial_match(const struct uuid *, const char *match);
void uuid_set_bits_v4(struct uuid *);

#endif /* __HAVE_UUID_H__ */
