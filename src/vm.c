#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void init_vm() {
    vm.cp = malloc(sizeof(int) * 1024);
    vm.tos = 0;
    vm.cpp = 0;
}

void free_vm() {
    free(vm.cp);
}

static void push(int value) {
    vm.stack[vm.tos++] = value;
}

static int pop() {
    return vm.stack[--vm.tos];
}

void run() {
#define BINARY_OP(op) \
do { \
    int b = pop(); \
    int a = pop(); \
    push(a op b); \
} while (false);
    while (true) {
        switch (*chunk.ip++) {
            case OP_PRINT: {
                int value = pop();
                printf("%d\n", value);
                break;
            }
            case OP_CONST: {
                push(vm.cp[*chunk.ip++]);
                break;
            }
            case OP_ADD: BINARY_OP(+); break;
            case OP_SUB: BINARY_OP(-); break;
            case OP_MUL: BINARY_OP(*); break;
            case OP_DIV: BINARY_OP(/); break;
            case OP_EXIT: return;
            default:
                break;
        }
    }
#undef BINARY_OP
}