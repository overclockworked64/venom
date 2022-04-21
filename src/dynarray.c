#include <stdlib.h>
#include "dynarray.h"

void dynarray_init(DynArray *array) {
    array->capacity = 0;
    array->count = 0;
    array->data = NULL;
}

void dynarray_insert(DynArray *array, Statement stmt) {
    if (array->count >= array->capacity) {
        array->capacity = array->capacity == 0 ? 2 : array->capacity * 2;
        array->data = realloc(array->data, sizeof(Statement) * array->capacity); 
    }
    array->data[array->count++] = stmt;
}

void dynarray_free(DynArray *array) {
    free(array->data);
}