#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void init_vm(VM *vm) {
    vm->cp = malloc(sizeof(double) * 1024);
    vm->tos = 0;
    vm->cpp = 0;
}

void free_vm(VM *vm) {
    free(vm->cp);
}

static void push(VM *vm, double value) {
    vm->stack[vm->tos++] = value;
}

static double pop(VM *vm) {
    return vm->stack[--vm->tos];
}

void run(VM *vm, BytecodeChunk *chunk) {
#define BINARY_OP(vm, op) \
do { \
    double b = pop(vm); \
    double a = pop(vm); \
    push(vm, a op b); \
} while (0)
    for (uint8_t *ip = chunk->code.data; ip < &chunk->code.data[chunk->code.count]; ) {
        switch (*ip++) {
            case OP_PRINT: {
                double value = pop(vm);
                printf("%f\n", value);
                break;
            }
            case OP_CONST: {
                push(vm, vm->cp[*ip++]);
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