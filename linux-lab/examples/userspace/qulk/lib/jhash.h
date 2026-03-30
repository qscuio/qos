#ifndef __HAVE_JHASH_H__
#define __HAVE_JHASH_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "util.h"

/* This is the public domain lookup3 hash by Bob Jenkins from
 * http://burtleburtle.net/bob/c/lookup3.c, modified for style.
 *
 * Use the functions in hash.h instead if you can.  These are here just for
 * places where we've exposed a hash function "on the wire" and don't want it
 * to change. */

uint32_t jhash_words(const uint32_t *, size_t n_word, uint32_t basis);
uint32_t jhash_bytes(const void *, size_t n_bytes, uint32_t basis);

#endif /* jhash.h */
