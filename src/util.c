#include "util.h"

char *own_string(const char *string) {
    char *s = malloc(strlen(string) + 1);
    strcpy(s, string);
    return s;
 }
