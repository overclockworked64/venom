#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void init_vm(VM *vm) {
    dynarray_init(&vm->cp);
    vm->tos = 0;
}

void free_vm(VM *vm) {
    dynarray_free(&vm->cp);
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
    /* operands are already on the stack */ \
    double b = pop(vm); \
    double a = pop(vm); \
    push(vm, a op b); \
} while (0)

    for (
        uint8_t *ip = chunk->code.data;
        ip < &chunk->code.data[chunk->code.count];  /* ip < addr of last instruction */
        ip++
    ) {
        switch (*ip) {  /* instruction pointer */
            case OP_PRINT: {
                double value = pop(vm);
                printf("%f\n", value);
                break;
            }
            case OP_CONST: {
                /* At this point, ip points to OP_CONST.
                 * We want to increment the ip to point to
                 * the index of the constant in the constant pool
                 * that comes after the opcode, and push the
                 * constant on the stack. */
                push(vm, vm->cp.data[*++ip]);
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