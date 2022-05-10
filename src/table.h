#ifndef venom_table_h
#define venom_table_h

#include "object.h"

typedef struct Bucket {
    char *key;
    Object obj;
    struct Bucket *next;
} Bucket;

typedef struct Table {
    Bucket *data[1024];
} Table;

void table_free(const Table *table);
void table_insert(Table *table, const char *key, Object obj);
Object *table_get(const Table *table, const char *key);

#endif