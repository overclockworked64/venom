#ifndef venom_dynarray_h
#define venom_dynarray_h

#include "parser.h"

typedef struct Statement Statement;

typedef struct DynArray {
    Statement *data;
    int count;
    int capacity;
} DynArray;

void dynarray_init(DynArray *array);
void dynarray_insert(DynArray *array, Statement stmt);
void dynarray_free(DynArray *array);

#endif