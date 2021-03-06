#include "str_array.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void str_tolower(char *str)
{
	for (; *str; ++str) {
		*str = (char)tolower(*str);
	}
}

void str_array_init(str_array *array)
{
	array->capacity = 4;
	array->length = 0;
	array->lines = calloc(array->capacity, sizeof(*array->lines));
	array->lower_lines =
	    calloc(array->capacity, sizeof(*array->lower_lines));
	array->lengths = calloc(array->capacity, sizeof(*array->lengths));
}

unsigned str_array_add(str_array *array, char *str)
{
	array->length++;
	if (array->length > array->capacity) {
		size_t new_cap = array->capacity * 3 / 2;
		void *pl =
		    realloc(array->lines, sizeof(*array->lines) * new_cap);
		void *pll = realloc(array->lower_lines,
				    sizeof(*array->lower_lines) * new_cap);
		void *plenghts =
		    realloc(array->lengths, sizeof(*array->lengths) * new_cap);
		if (!pl || !pll || !plenghts) {
			array->length--;
			free(pl);
			free(pll);
			free(plenghts);
			return 0;
		}
		array->lines = pl;
		array->lower_lines = pll;
		array->lengths = plenghts;
		array->capacity = new_cap;
	}
	array->lines[array->length - 1] = str;
	char *lower = strdup(str);
	str_tolower(lower);
	array->lower_lines[array->length - 1] = lower;
	array->lengths[array->length - 1] = (unsigned)strlen(str);
	return 1;
}

void str_array_free(str_array *array)
{
	for (size_t i = 0; i < array->length; ++i) {
		free(array->lines[i]);
		free(array->lower_lines[i]);
	}
	free(array->lines);
	free(array->lower_lines);
	free(array->lengths);
}
