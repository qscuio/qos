#ifndef __HAVE_AES128_H__
#define __HAVE_AES128_H__

#include <stdint.h>

struct aes128 {
    uint32_t rk[128/8 + 28];
};

void aes128_schedule(struct aes128 *, const uint8_t key[16]);
void aes128_encrypt(const struct aes128 *, const void *, void *);

#endif  /* __HAVE_AES128_H__ */
