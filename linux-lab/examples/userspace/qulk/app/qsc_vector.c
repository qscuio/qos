#include "qsc_vector.h"

int main(int argc, const char* argv[]) {
	qsc_vector vector;
	int x, y, sum;

	/* Choose initial capacity of 10 */
	/* Specify the size of the elements you want to store once */
	qsc_vector_setup(&vector, 10, sizeof(int));

	x = 6, y = 9;
	qsc_vector_push_back(&vector, &x);
	qsc_vector_insert(&vector, 0, &y);
	qsc_vector_assign(&vector, 0, &y);

	x = *(int*)qsc_vector_get(&vector, 0);
	y = *(int*)qsc_vector_back(&vector);
	x = QSC_VECTOR_GET_AS(int, &vector, 1);

	qsc_vector_erase(&vector, 1);

	/* Iterator support */
	Iterator iterator = qsc_vector_begin(&vector);
	Iterator last = qsc_vector_end(&vector);
	for (; !iterator_equals(&iterator, &last); iterator_increment(&iterator)) {
		*(int*)iterator_get(&iterator) += 1;
	}

	/* Or just use pretty macros */
	sum = 0;
	QSC_VECTOR_FOR_EACH(&vector, i) {
		sum += ITERATOR_GET_AS(int, &i);
	}

	/* Memory management interface */
	qsc_vector_resize(&vector, 10);
	qsc_vector_reserve(&vector, 100);

	qsc_vector_clear(&vector);
	qsc_vector_destroy(&vector);
}
