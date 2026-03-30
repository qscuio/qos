#ifndef __HAVE_QSC_TRIE_H__

#define __HAVE_QSC_TRIE_H__

/**
 * C99 Trie Library
 *
 * This trie associates an arbitrary void pointer with a NUL-terminated
 * C string key. All lookups are O(n), n being the length of the string.
 * Strings are stored in sorted order, so visitor functions visit keys
 * in lexicographical order. The visitor can also be used to visit keys
 * by a string prefix. An empty prefix "" matches all keys (the prefix
 * argument should never be NULL).
 *
 * Except for trie_free() and trie_prune(), memory is never freed by the
 * trie, even when entries are "removed" by associating a NULL pointer.
 *
 * @see http://en.wikipedia.org/wiki/Trie
 */

#include "qsc_compiler.h"

struct qsc_trie;
struct qsc_trie_it;

typedef int (*qsc_trie_visitor)(const char *key, void *data, void *arg);
typedef void *(*qsc_trie_replacer)(const char *key, void *current, void *arg);

/**
 * @return a freshly allocated trie, NULL on allocation error
 */
struct qsc_trie *qsc_trie_create(void);

/**
 * Destroys a trie created by trie_create().
 * @return 0 on success
 */
int qsc_trie_free(struct qsc_trie *);

/**
 * Finds for the data associated with KEY.
 * @return the previously inserted data
 */
void *qsc_trie_search(const struct qsc_trie *, const char *key);

/**
 * Insert or replace DATA associated with KEY. Inserting NULL is the
 * equivalent of unassociating that key, though no memory will be
 * released.
 * @return 0 on success
 */
int qsc_trie_insert(struct qsc_trie *, const char *key, void *data);

/**
 * Replace data associated with KEY using a replacer function. The
 * replacer function gets the key, the original data (NULL if none)
 * and ARG. Its return value is inserted into the trie.
 * @return 0 on success
 */
int qsc_trie_replace(struct qsc_trie *, const char *key, qsc_trie_replacer f, void *arg);

/**
 * Visit in lexicographical order each key that matches the prefix. An
 * empty prefix visits every key in the trie. The visitor must accept
 * three arguments: the key, the data, and ARG. Iteration is aborted
 * (with success) if visitor returns non-zero.
 * @return 0 on success
 */
int qsc_trie_visit(struct qsc_trie *, const char *prefix, qsc_trie_visitor v, void *arg);

/**
 * Remove all unused branches in a trie.
 * @return 0 on success
 */
int qsc_trie_prune(struct qsc_trie *);

/**
 * Count the number of entries with a given prefix. An empty prefix
 * counts the entire trie.
 * @return the number of entries matching PREFIX
 */
size_t qsc_trie_count(struct qsc_trie *, const char *prefix);

/**
 * Compute the total memory usage of a trie.
 * @return the size in bytes, or 0 on error
 */
size_t qsc_trie_size(struct qsc_trie *);

/**
 * Create an iterator that visits each key with the given prefix, in
 * lexicographical order. Making any modifications to the trie
 * invalidates the iterator.
 * @return a fresh iterator pointing to the first key
 */
struct qsc_trie_it *qsc_trie_it_create(struct qsc_trie *, const char *prefix);

/**
 * Advance iterator to the next key in the sequence.
 * @return 0 if done, else 1
 */
int qsc_trie_it_next(struct qsc_trie_it *);

/**
 * Returned buffer is invalidated on the next trie_it_next().
 * @return a buffer containing the current key
 */
const char *qsc_trie_it_key(struct qsc_trie_it *);

/**
 * @return the data pointer for the current key
 */
void *qsc_trie_it_data(struct qsc_trie_it *);

/**
 * @return 1 if the iterator has completed (including errors)
 */
int qsc_trie_it_done(struct qsc_trie_it *);

/**
 * @return 1 if the iterator experienced an error
 */
int qsc_trie_it_error(struct qsc_trie_it *);

/**
 * Destroys the iterator.
 * @return 0 on success
 */
void qsc_trie_it_free(struct qsc_trie_it *);

#endif /* __HAVE_QSC_TRIE_H__ */
