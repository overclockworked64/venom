#ifndef venom_table_h
#define venom_table_h

typedef struct Bucket {
    char *key;
    double value;
    struct Bucket *next;
} Bucket;

typedef struct Table {
    Bucket *data[1024];
} Table;

void table_free(Table *table);
void table_insert(Table *table, char *key, double value);
double table_get(Table *table, char *key);

#endif