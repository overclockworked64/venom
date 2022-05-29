#ifndef venom_object_h
#define venom_object_h

#include <stdbool.h>

typedef enum {
    OBJ_NUMBER,
    OBJ_BOOLEAN,
} ObjectType;

typedef struct {
    ObjectType type;
    union {
        double dval;
        bool bval;
    } value;
} Object;

#define IS_BOOL(object) ((object)->type == OBJ_BOOLEAN)
#define IS_NUM(object) ((object)->type == OBJ_NUMBER)

#define AS_NUM(thing) ((Object){ .type = OBJ_NUMBER, .value.dval = (thing)})
#define AS_BOOL(thing) ((Object){ .type = OBJ_BOOLEAN, .value.bval = (thing)})

#define NUM_VAL(object) ((object).value.dval)
#define BOOL_VAL(object) ((object).value.bval)

void print_object(Object *object);

#endif