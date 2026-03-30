#include <stdlib.h>
#include <string.h>
#include "qsc_trie.h"

struct qsc_trieptr {
    struct qsc_trie *trie;
    int c;
};

struct qsc_trie {
    void *data;
    short nchildren, size;
    struct qsc_trieptr children[];
};

/* Mini stack library for non-recursive traversal. */

struct qsc_stack_node {
    struct qsc_trie *trie;
    int i;
};

struct qsc_stack {
    struct qsc_stack_node *stack;
    size_t fill, size;
};

static int qsc_stack_init(struct qsc_stack *s)
{
    s->size = 256;
    s->fill = 0;
    s->stack = malloc(s->size * sizeof(struct qsc_stack_node));
    return !s->stack ? -1 : 0;
}

static void qsc_stack_free(struct qsc_stack *s)
{
    free(s->stack);
    s->stack = 0;
}

static int qsc_stack_grow(struct qsc_stack *s)
{
    size_t newsize = s->size * 2 * sizeof(struct qsc_stack_node);
    struct qsc_stack_node *resize = realloc(s->stack, newsize);
    if (!resize) {
        qsc_stack_free(s);
        return -1;
    }
    s->size *= 2;
    s->stack = resize;
    return 0;
}

static int qsc_stack_push(struct qsc_stack *s, struct qsc_trie *trie)
{
    if (s->fill == s->size)
        if (qsc_stack_grow(s) != 0)
            return -1;
    s->stack[s->fill++] = (struct qsc_stack_node){trie, 0};
    return 0;
}

static struct qsc_trie *qsc_stack_pop(struct qsc_stack *s)
{
    return s->stack[--s->fill].trie;
}

static struct qsc_stack_node *qsc_stack_peek(struct qsc_stack *s)
{
    return &s->stack[s->fill - 1];
}

/* Constructor and destructor. */

struct qsc_trie *qsc_trie_create(void)
{
    /* Root never needs to be resized. */
    size_t tail_size = sizeof(struct qsc_trieptr) * 255;
    struct qsc_trie *root = malloc(sizeof(*root) + tail_size);
    if (!root)
        return 0;
    root->size = 255;
    root->nchildren = 0;
    root->data = 0;
    return root;
}

int qsc_trie_free(struct qsc_trie *trie)
{
    struct qsc_stack stack, *s = &stack;
    if (qsc_stack_init(s) != 0)
        return 1;
    qsc_stack_push(s, trie); /* first push always successful */
    while (s->fill > 0) {
        struct qsc_stack_node *node = qsc_stack_peek(s);
        if (node->i < node->trie->nchildren) {
            int i = node->i++;
            if (qsc_stack_push(s, node->trie->children[i].trie) != 0)
                return 1;
        } else {
            free(qsc_stack_pop(s));
        }
    }
    qsc_stack_free(s);
    return 0;
}

/* Core search functions. */

static size_t qsc_binary_search(struct qsc_trie *self, struct qsc_trie **child,
              struct qsc_trieptr **ptr, const unsigned char *key)
{
    size_t i = 0;
    int found = 1;
    *ptr = 0;
    while (found && key[i]) {
        int first = 0;
        int last = self->nchildren - 1;
        int middle;
        found = 0;
        while (first <= last) {
            middle = (first + last) / 2;
            struct qsc_trieptr *p = &self->children[middle];
            if (p->c < key[i]) {
                first = middle + 1;
            } else if (p->c == key[i]) {
                self = p->trie;
                *ptr = p;
                found = 1;
                i++;
                break;
            } else {
                last = middle - 1;
            }
        }
    }
    *child = self;
    return i;
}

void * qsc_trie_search(const struct qsc_trie *self, const char *key)
{
    struct qsc_trie *child;
    struct qsc_trieptr *parent;
    unsigned char *ukey = (unsigned char *)key;
    size_t depth = qsc_binary_search((struct qsc_trie *)self, &child, &parent, ukey);
    return !key[depth] ? child->data : 0;
}

/* Insertion functions. */

static struct qsc_trie *qsc_grow(struct qsc_trie *self) 
{
    int size = self->size * 2;
    if (size > 255)
        size = 255;
    size_t children_size = sizeof(struct qsc_trieptr) * size;
    struct qsc_trie *resized = realloc(self, sizeof(*self) + children_size);
    if (!resized)
        return 0;
    resized->size = size;
    return resized;
}

static int qsc_ptr_cmp(const void *a, const void *b)
{
    return ((struct qsc_trieptr *)a)->c - ((struct qsc_trieptr *)b)->c;
}

static struct qsc_trie *qsc_node_add(struct qsc_trie *self, int c, struct qsc_trie *child)
{
    if (self->size == self->nchildren) {
        self = qsc_grow(self);
        if (!self)
            return 0;
    }
    int i = self->nchildren++;
    self->children[i].c = c;
    self->children[i].trie = child;
    qsort(self->children, self->nchildren, sizeof(self->children[0]), qsc_ptr_cmp);
    return self;
}

static void qsc_node_remove(struct qsc_trie *self, int i)
{
    size_t len = (--self->nchildren - i) * sizeof(self->children[0]);
    memmove(self->children + i, self->children + i + 1, len);
}

static struct qsc_trie * qsc_create(void)
{
    int size = 1;
    struct qsc_trie *trie = malloc(sizeof(*trie) + sizeof(struct qsc_trieptr) * size);
    if (!trie)
        return 0;
    trie->size = size;
    trie->nchildren = 0;
    trie->data = 0;
    return trie;
}

static void *qsc_identity(const char *key, void *data, void *arg)
{
    (void)key;
    (void)data;
    return arg;
}

int qsc_trie_replace(struct qsc_trie *self, const char *key, qsc_trie_replacer f, void *arg)
{
    struct qsc_trie *last;
    struct qsc_trieptr *parent;
    unsigned char *ukey = (unsigned char *)key;
    size_t depth = qsc_binary_search(self, &last, &parent, ukey);
    while (ukey[depth]) {
        struct qsc_trie *subtrie = qsc_create();
        if (!subtrie)
            return 1;
        struct qsc_trie *added = qsc_node_add(last, ukey[depth], subtrie);
        if (!added) {
            free(subtrie);
            return 1;
        }
        if (parent) {
            parent->trie = added;
            parent = 0;
        }
        last = subtrie;
        depth++;
    }
    last->data = f(key, last->data, arg);
    return 0;
}

int qsc_trie_insert(struct qsc_trie *trie, const char *key, void *data)
{
    return qsc_trie_replace(trie, key, qsc_identity, data);
}

/* Mini buffer library. */

struct qsc_buffer {
    char *buffer;
    size_t size, fill;
};

static int qsc_buffer_init(struct qsc_buffer *b, const char *prefix)
{
    b->fill = strlen(prefix);
    b->size = b->fill >= 256 ? b->fill * 2 : 256;
    b->buffer = malloc(b->size);
    if (b->buffer)
        memcpy(b->buffer, prefix, b->fill + 1);
    return !b->buffer ? -1 : 0;
}

static void qsc_buffer_free(struct qsc_buffer *b)
{
    free(b->buffer);
    b->buffer = 0;
}

static int qsc_buffer_grow(struct qsc_buffer *b)
{
    char *resize = realloc(b->buffer, b->size * 2);
    if (!resize) {
        qsc_buffer_free(b);
        return -1;
    }
    b->buffer = resize;
    b->size *= 2;
    return 0;
}

static int qsc_buffer_push(struct qsc_buffer *b, char c)
{
    if (b->fill + 1 == b->size)
        if (qsc_buffer_grow(b) != 0)
            return -1;
    b->buffer[b->fill++] = c;
    b->buffer[b->fill] = 0;
    return 0;
}

static void qsc_buffer_pop(struct qsc_buffer *b)
{
    if (b->fill > 0)
        b->buffer[--b->fill] = 0;
}

/* Core visitation functions. */

static int qsc_visit(struct qsc_trie *self, const char *prefix, qsc_trie_visitor visitor, void *arg)
{
    struct qsc_buffer buffer, *b = &buffer;
    struct qsc_stack stack, *s = &stack;
    if (qsc_buffer_init(b, prefix) != 0)
        return -1;
    if (qsc_stack_init(s) != 0) {
        qsc_buffer_free(b);
        return -1;
    }
    qsc_stack_push(s, self);
    while (s->fill > 0) {
        struct qsc_stack_node *node = qsc_stack_peek(s);
        if (node->i == 0 && node->trie->data) {
            if (visitor(b->buffer, node->trie->data, arg) != 0) {
                qsc_buffer_free(b);
                qsc_stack_free(s);
                return 1;
            }
        }
        if (node->i < node->trie->nchildren) {
            struct qsc_trie *trie = node->trie->children[node->i].trie;
            int c = node->trie->children[node->i].c;
            node->i++;
            if (qsc_stack_push(s, trie) != 0) {
                qsc_buffer_free(b);
                return -1;
            }
            if (qsc_buffer_push(b, c) != 0) {
                qsc_stack_free(s);
                return -1;
            }
        } else {
            qsc_buffer_pop(b);
            qsc_stack_pop(s);
        }
    }
    qsc_buffer_free(b);
    qsc_stack_free(s);
    return 0;
}

int
qsc_trie_visit(struct qsc_trie *self, const char *prefix, qsc_trie_visitor v, void *arg)
{
    struct qsc_trie *start = self;
    struct qsc_trieptr *ptr;
    unsigned char *uprefix = (unsigned char *)prefix;
    int depth = qsc_binary_search(self, &start, &ptr, uprefix);
    if (prefix[depth])
        return 0;
    int r = qsc_visit(start, prefix, v, arg);
    return r >= 0 ? 0 : -1;
}

/* Miscellaneous functions. */

static int qsc_visitor_counter(const char *key, void *data, void *arg)
{
    (void) key;
    (void) data;
    size_t *count = arg;
    count[0]++;
    return 0;
}

size_t qsc_trie_count(struct qsc_trie *trie, const char *prefix)
{
    size_t count = 0;
    qsc_trie_visit(trie, prefix, qsc_visitor_counter, &count);
    return count;
}

int qsc_trie_prune(struct qsc_trie *trie)
{
    struct qsc_stack stack, *s = &stack;
    if (qsc_stack_init(s) != 0)
        return -1;
    qsc_stack_push(s, trie);
    while (s->fill > 0) {
        struct qsc_stack_node *node = qsc_stack_peek(s);
        int i = node->i++;
        if (i < node->trie->nchildren) {
            if (qsc_stack_push(s, node->trie->children[i].trie) != 0)
                return 0;
        } else {
            struct qsc_trie *t = qsc_stack_pop(s);
            for (int i = 0; i < t->nchildren; i++) {
                struct qsc_trie *child = t->children[i].trie;
                if (!child->nchildren && !child->data) {
                    qsc_node_remove(t, i--);
                    free(child);
                }
            }
        }
    }
    qsc_stack_free(s);
    return 1;
}

size_t qsc_trie_size(struct qsc_trie *trie)
{
    struct qsc_stack stack, *s = &stack;
    if (qsc_stack_init(s) != 0)
        return 0;
    qsc_stack_push(s, trie);
    size_t size = 0;
    while (s->fill > 0) {
        struct qsc_stack_node *node = qsc_stack_peek(s);
        int i = node->i++;
        if (i < node->trie->nchildren) {
            if (qsc_stack_push(s, node->trie->children[i].trie) != 0)
                return 0;
        } else {
            struct qsc_trie *t = qsc_stack_pop(s);
            size += sizeof(*t) + sizeof(*t->children) * t->size;
        }
    }
    qsc_stack_free(s);
    return size;
}

struct qsc_trie_it {
    struct qsc_stack stack;
    struct qsc_buffer buffer;
    void *data;
    int error;
};

struct qsc_trie_it *qsc_trie_it_create(struct qsc_trie *trie, const char *prefix)
{
    struct qsc_trie_it *it = malloc(sizeof(*it));
    if (!it)
        return 0;
    if (qsc_stack_init(&it->stack)) {
        free(it);
        return 0;
    }
    if (qsc_buffer_init(&it->buffer, prefix)) {
        qsc_stack_free(&it->stack);
        free(it);
        return 0;
    }
    qsc_stack_push(&it->stack, trie); /* first push always successful */
    it->data = 0;
    it->error = 0;
    qsc_trie_it_next(it);
    return it;
}

int qsc_trie_it_next(struct qsc_trie_it *it)
{
    while (!it->error && it->stack.fill) {
        struct qsc_stack_node *node = qsc_stack_peek(&it->stack);

        if (node->i == 0 && node->trie->data) {
            if (!it->data) {
                it->data = node->trie->data;
                return 1;
            } else {
                it->data = 0;
            }
        }

        if (node->i < node->trie->nchildren) {
            struct qsc_trie *trie = node->trie->children[node->i].trie;
            int c = node->trie->children[node->i].c;
            node->i++;
            if (qsc_stack_push(&it->stack, trie)) {
                it->error = 1;
                return 0;
            }
            if (qsc_buffer_push(&it->buffer, c)) {
                it->error = 1;
                return 0;
            }
        } else {
            qsc_buffer_pop(&it->buffer);
            qsc_stack_pop(&it->stack);
        }
    }
    return 0;
}

const char * qsc_trie_it_key(struct qsc_trie_it *it)
{
    return it->buffer.buffer;
}

void *qsc_trie_it_data(struct qsc_trie_it *it)
{
    return it->data;
}

int qsc_trie_it_done(struct qsc_trie_it *it)
{
    return it->error || !it->stack.fill;
}

int qsc_trie_it_error(struct qsc_trie_it *it)
{
    return it->error;
}

void qsc_trie_it_free(struct qsc_trie_it *it)
{
    qsc_buffer_free(&it->buffer);
    qsc_stack_free(&it->stack);
    free(it);
}
