#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void push(int value) {
    stack[tos++] = value;
}

int pop() {
    return stack[--tos];
}

void run() {
#define BINARY_OP(op) \
do { \
    int b = pop(); \
    int a = pop(); \
    push(a op b); \
} while (false);
    tos = 0;
    while (true) {
        switch (*compiling_chunk.ip++) {
            case OP_PRINT: {
                int value = pop();
                printf("%d\n", value);
                break;
            }
            case OP_CONST: {
                push(*compiling_chunk.ip++);
                break;
            }
            case OP_ADD: BINARY_OP(+); break;
            case OP_SUB: BINARY_OP(-); break;
            case OP_MUL: BINARY_OP(*); break;
            case OP_DIV: BINARY_OP(/); break;
            case OP_EXIT: exit(0);
            default:
                break;
        }
    }
#undef BINARY_OP
}