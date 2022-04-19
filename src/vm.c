#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void init_vm(VM *vm) {
    vm->cp = malloc(sizeof(int) * 1024);
    vm->tos = 0;
    vm->cpp = 0;
}

void free_vm(VM *vm) {
    free(vm->cp);
}

static void push(VM *vm, int value) {
    vm->stack[vm->tos++] = value;
}

static int pop(VM *vm) {
    return vm->stack[--vm->tos];
}

void run(VM *vm, BytecodeChunk *chunk) {
#define BINARY_OP(vm, op) \
do { \
    int b = pop(vm); \
    int a = pop(vm); \
    push(vm, a op b); \
} while (false);
    while (true) {
        switch (*chunk->ip++) {
            case OP_PRINT: {
                int value = pop(vm);
                printf("%d\n", value);
                break;
            }
            case OP_CONST: {
                push(vm, vm->cp[*chunk->ip++]);
                break;
            }
            case OP_ADD: BINARY_OP(vm, +); break;
            case OP_SUB: BINARY_OP(vm, -); break;
            case OP_MUL: BINARY_OP(vm, *); break;
            case OP_DIV: BINARY_OP(vm, /); break;
            case OP_EXIT: return;
            default:
                break;
        }
    }
#undef BINARY_OP
}