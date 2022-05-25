#ifndef venom_object_h
#define venom_object_h

#include <stdbool.h>
#include <stdint.h>
#include "dynarray.h"

typedef enum {
    OBJ_NUMBER,
    OBJ_BOOLEAN,
    OBJ_FUNCTION,
    OBJ_POINTER,
} ObjectType;

typedef struct Object Object;

typedef struct {
    char *name;
    uint8_t location;
    size_t paramcount;
} Function;

typedef struct Object {
    ObjectType type;
    union {
        double dval;
        bool bval;
        Function func;
        uint8_t *ptr;
    } as;
    char *name;
} Object;

typedef DynArray(Object) Object_DynArray;
typedef DynArray(char *) String_DynArray;

#define IS_BOOL(object) ((object)->type == OBJ_BOOLEAN)
#define IS_NUM(object) ((object)->type == OBJ_NUMBER)
#define IS_FUNC(object) ((object)->type == OBJ_FUNCTION)
#define IS_POINTER(object) ((object)->type == OBJ_POINTER)

#define AS_NUM(thing) ((Object){ .type = OBJ_NUMBER, .as.dval = (thing) })
#define AS_BOOL(thing) ((Object){ .type = OBJ_BOOLEAN, .as.bval = (thing) })
#define AS_FUNC(thing) ((Object){ .type = OBJ_FUNCTION, .as.func = (thing) })
#define AS_POINTER(thing) ((Object){ .type = OBJ_POINTER, .as.ptr = (thing)} )

#define NUM_VAL(object) ((object).as.dval)
#define BOOL_VAL(object) ((object).as.bval)

void print_object(Object *object);

#endif