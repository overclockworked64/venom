#include <stdio.h>
#include "util.h"

char *own_string(const char *string) {
    char *s = malloc(strlen(string)+1);
    snprintf(s, strlen(string)+1, "%s", string);
    return s;
 }

 char *own_string_n(const char *string, int n) {
    char *s = malloc(strlen(string)+1);
    snprintf(s, n+1, "%s", string);
    return s;
 }
