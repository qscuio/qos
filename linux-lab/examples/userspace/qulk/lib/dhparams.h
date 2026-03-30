#ifndef __HAVE_DHPARAMS_H__
#define __HAVE_DHPARAMS_H__

#include <inttypes.h>
#include <openssl/dh.h>

DH *get_dh2048(void);
DH *get_dh4096(void);

#endif /* __HAVE_DHPARAMS_H__  */
