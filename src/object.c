#include <stdio.h>
#include "object.h"

void print_object(Object *object) {
    if IS_BOOL(object) {
        printf("%s\n", object->value.bval ? "true" : "false");
    } else if IS_NUM(object) {
        printf("%.2f\n", object->value.dval);
    }
}