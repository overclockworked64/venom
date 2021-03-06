#ifndef venom_dynarray_h
#define venom_dynarray_h

#include <stddef.h>

#define DynArray(T) struct { \
    T *data; \
    size_t count; \
    size_t capacity; \
}

#define dynarray_insert(array, exp) \
do { \
    if ((array)->count >= (array)->capacity) { \
        (array)->capacity = (array)->capacity == 0 ? 2 : (array)->capacity * 2; \
        (array)->data = realloc((array)->data, sizeof((array)->data[0]) * (array)->capacity); \
    } \
    (array)->data[(array)->count++] = (exp); \
} while (0)

#define dynarray_free(array) \
do { \
    free((array)->data); \
} while (0)

typedef DynArray(int) IntDynArray;

#endif