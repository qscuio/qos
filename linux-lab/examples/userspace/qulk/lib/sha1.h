#ifndef __HAVE_SHA1_H__
#define __HAVE_SHA1_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SHA1_DIGEST_SIZE 20     /* Size of the SHA1 digest. */
#define SHA1_HEX_DIGEST_LEN 40  /* Length of SHA1 digest as hex in ASCII. */

/* SHA1 context structure. */
struct sha1_ctx {
    uint32_t digest[5];          /* Message digest. */
    uint32_t count_lo, count_hi; /* 64-bit bit counts. */
    uint32_t data[16];           /* SHA data buffer */
    int local;                   /* Unprocessed amount in data. */
};

void sha1_init(struct sha1_ctx *);
void sha1_update(struct sha1_ctx *, const void *, uint32_t size);
void sha1_final(struct sha1_ctx *, uint8_t digest[SHA1_DIGEST_SIZE]);
void sha1_bytes(const void *, uint32_t size, uint8_t digest[SHA1_DIGEST_SIZE]);

#define SHA1_FMT \
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x" \
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
#define SHA1_ARGS(DIGEST) \
    ((DIGEST)[0]), ((DIGEST)[1]), ((DIGEST)[2]), ((DIGEST)[3]), \
    ((DIGEST)[4]), ((DIGEST)[5]), ((DIGEST)[6]), ((DIGEST)[7]), \
    ((DIGEST)[8]), ((DIGEST)[9]), ((DIGEST)[10]), ((DIGEST)[11]), \
    ((DIGEST)[12]), ((DIGEST)[13]), ((DIGEST)[14]), ((DIGEST)[15]), \
    ((DIGEST)[16]), ((DIGEST)[17]), ((DIGEST)[18]), ((DIGEST)[19])

void sha1_to_hex(const uint8_t digest[SHA1_DIGEST_SIZE],
                 char hex[SHA1_HEX_DIGEST_LEN + 1]);
bool sha1_from_hex(uint8_t digest[SHA1_DIGEST_SIZE], const char *hex);

#endif  /* __HAVE_SHA1_H__ */
