#ifndef __HAVE_JHASH_H__
#define __HAVE_JHASH_H__

/* The most generic version, hashes an arbitrary sequence
 * of bytes.  No alignment or length assumptions are made about
 * the input key.
 */
extern uint32_t jhash(const void *key, uint32_t length, uint32_t initval);

/* A special optimized version that handles 1 or more of uint32_ts.
 * The length parameter here is the number of uint32_ts in the key.
 */
extern uint32_t jhash2(const uint32_t *k, uint32_t length, uint32_t initval);

/* A special ultra-optimized versions that knows they are hashing exactly
 * 3, 2 or 1 word(s).
 *
 * NOTE: In partilar the "c += length; __jhash_mix(a,b,c);" normally
 *       done at the end is not done here.
 */
extern uint32_t jhash_3words(uint32_t a, uint32_t b, uint32_t c,
			     uint32_t initval);
extern uint32_t jhash_2words(uint32_t a, uint32_t b, uint32_t initval);
extern uint32_t jhash_1word(uint32_t a, uint32_t initval);

#endif /* __HAVE_JHASH_H__ */
