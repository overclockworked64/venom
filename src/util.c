#include "util.h"

char *own_string(const char *string) {
    char *s = malloc(strlen(string) + 1);
    strcpy(s, string);
    return s;
 }

 char *own_string_n(const char *string, int n) {
    char *s = malloc(strlen(string) + 1);
    strncpy(s, string, n);
    return s;
 }
