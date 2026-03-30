#ifndef __HAVE_QSC_QSC_VECTORH__

#define __HAVE_QSC_QSC_VECTORH__

// The MIT License (MIT)
// Copyright (c) 2016 Peter Goldsborough
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <stdbool.h>
#include <stddef.h>

/***** DEFINITIONS *****/

#define QSC_VECTOR_MINIMUM_CAPACITY 2
#define QSC_VECTOR_GROWTH_FACTOR 2
#define QSC_VECTOR_SHRINK_THRESHOLD (1 / 4)

#define QSC_VECTOR_ERROR -1
#define QSC_VECTOR_SUCCESS 0

#define QSC_VECTOR_UNINITIALIZED NULL
#define QSC_VECTOR_INITIALIZER \
	{ 0, 0, 0, QSC_VECTOR_UNINITIALIZED }

/***** STRUCTURES *****/

typedef struct qsc_vector {
	size_t size;
	size_t capacity;
	size_t element_size;

	void* data;
} qsc_vector;

typedef struct Iterator {
	void* pointer;
	size_t element_size;
} Iterator;

/***** METHODS *****/

/* Constructor */
int qsc_vector_setup(qsc_vector* vector, size_t capacity, size_t element_size);

/* Copy Constructor */
int qsc_vector_copy(qsc_vector* destination, qsc_vector* source);

/* Copy Assignment */
int qsc_vector_copy_assign(qsc_vector* destination, qsc_vector* source);

/* Move Constructor */
int qsc_vector_move(qsc_vector* destination, qsc_vector* source);

/* Move Assignment */
int qsc_vector_move_assign(qsc_vector* destination, qsc_vector* source);

int qsc_vector_swap(qsc_vector* destination, qsc_vector* source);

/* Destructor */
int qsc_vector_destroy(qsc_vector* vector);

/* Insertion */
int qsc_vector_push_back(qsc_vector* vector, void* element);
int qsc_vector_push_front(qsc_vector* vector, void* element);
int qsc_vector_insert(qsc_vector* vector, size_t index, void* element);
int qsc_vector_assign(qsc_vector* vector, size_t index, void* element);

/* Deletion */
int qsc_vector_pop_back(qsc_vector* vector);
int qsc_vector_pop_front(qsc_vector* vector);
int qsc_vector_erase(qsc_vector* vector, size_t index);
int qsc_vector_clear(qsc_vector* vector);

/* Lookup */
void* qsc_vector_get(qsc_vector* vector, size_t index);
const void* qsc_vector_const_get(const qsc_vector* vector, size_t index);
void* qsc_vector_front(qsc_vector* vector);
void* qsc_vector_back(qsc_vector* vector);
#define QSC_VECTOR_GET_AS(type, qsc_vector_pointer, index) \
	*((type*)qsc_vector_get((qsc_vector_pointer), (index)))

/* Information */
bool qsc_vector_is_initialized(const qsc_vector* vector);
size_t qsc_vector_byte_size(const qsc_vector* vector);
size_t qsc_vector_free_space(const qsc_vector* vector);
bool qsc_vector_is_empty(const qsc_vector* vector);

/* Memory management */
int qsc_vector_resize(qsc_vector* vector, size_t new_size);
int qsc_vector_reserve(qsc_vector* vector, size_t minimum_capacity);
int qsc_vector_shrink_to_fit(qsc_vector* vector);

/* Iterators */
Iterator qsc_vector_begin(qsc_vector* vector);
Iterator qsc_vector_end(qsc_vector* vector);
Iterator qsc_vector_iterator(qsc_vector* vector, size_t index);

void* iterator_get(Iterator* iterator);
#define ITERATOR_GET_AS(type, iterator) *((type*)iterator_get((iterator)))

int iterator_erase(qsc_vector* vector, Iterator* iterator);

void iterator_increment(Iterator* iterator);
void iterator_decrement(Iterator* iterator);

void* iterator_next(Iterator* iterator);
void* iterator_previous(Iterator* iterator);

bool iterator_equals(Iterator* first, Iterator* second);
bool iterator_is_before(Iterator* first, Iterator* second);
bool iterator_is_after(Iterator* first, Iterator* second);

size_t iterator_index(qsc_vector* vector, Iterator* iterator);

#define QSC_VECTOR_FOR_EACH(qsc_vector_pointer, iterator_name)           \
	for (Iterator(iterator_name) = qsc_vector_begin((qsc_vector_pointer)), \
			end = qsc_vector_end((qsc_vector_pointer));                        \
			 !iterator_equals(&(iterator_name), &end);                 \
			 iterator_increment(&(iterator_name)))

/***** PRIVATE *****/

#define MAX(a, b) ((a) > (b) ? (a) : (b))

bool _qsc_vector_should_grow(qsc_vector* vector);
bool _qsc_vector_should_shrink(qsc_vector* vector);

size_t _qsc_vector_free_bytes(const qsc_vector* vector);
void* _qsc_vector_offset(qsc_vector* vector, size_t index);
const void* _qsc_vector_const_offset(const qsc_vector* vector, size_t index);

void _qsc_vector_assign(qsc_vector* vector, size_t index, void* element);

int _qsc_vector_move_right(qsc_vector* vector, size_t index);
void _qsc_vector_move_left(qsc_vector* vector, size_t index);

int _qsc_vector_adjust_capacity(qsc_vector* vector);
int _qsc_vector_reallocate(qsc_vector* vector, size_t new_capacity);

void _qsc_vector_swap(size_t* first, size_t* second);

#endif /* __HAVE_QSC_QSC_VECTORH__ */
