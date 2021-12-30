#include "str_array.h"
#include <stdlib.h>

void str_array_init(str_array *array)
{
	array->capacity = 4;
	array->length = 0;
	array->lines = calloc(array->capacity, sizeof(*array->lines));
}

unsigned str_array_add(str_array *array, char *str)
{
	array->length++;
	if (array->length > array->capacity) {
		size_t new_cap = array->capacity * 3 / 2;
		void *p =
		    realloc(array->lines, sizeof(*array->lines) * new_cap);
		if (!p) {
			array->length--;
			return 0;
		}
		array->lines = p;
		array->capacity = new_cap;
	}
	array->lines[array->length - 1] = str;
	return 1;
}

void str_array_free(str_array *array)
{
	for (size_t i = 0; i < array->length; ++i) {
		free(array->lines[i]);
	}
	free(array->lines);
}
