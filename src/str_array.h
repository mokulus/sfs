#ifndef STR_ARRAY_H
#define STR_ARRAY_H

#include <unistd.h>

typedef struct {
	char **lines;
	char **lower_lines;
	unsigned *lengths;
	size_t length;
	size_t capacity;
} str_array;

void str_array_init(str_array *array);
unsigned str_array_add(str_array *array, char *str);
void str_array_free(str_array *array);

#endif
