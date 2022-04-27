#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "compiler.h"
#include "vm.h"

#define venom_debug

void init_chunk(BytecodeChunk *chunk) {
    dynarray_init(&chunk->code);
}

void free_chunk(BytecodeChunk *chunk) {
    dynarray_free(&chunk->code);
}

int add_string(VM *vm, char *string) {
    dynarray_insert(&vm->sp, string);
    return vm->sp.count - 1;
}

int add_constant(VM *vm, double constant) {
    dynarray_insert(&vm->cp, constant);
    return vm->cp.count - 1;
}

void emit_byte(BytecodeChunk *chunk, uint8_t byte) {
    dynarray_insert(&chunk->code, byte);
}

void emit_bytes(BytecodeChunk *chunk, int n, ...) {
    va_list ap;
    va_start(ap, n);

    for (int i = 0; i < n; ++i) {
        uint8_t byte = va_arg(ap, int);
        emit_byte(chunk, byte);
    }

    va_end(ap);
}

void compile_expression(BytecodeChunk *chunk, VM *vm, Expression exp) {
    if (exp.kind == LITERAL) {
        int index = add_constant(vm, exp.data.dval);
        emit_bytes(chunk, 2, OP_CONST, index);
    } else if (exp.kind == UNARY) {
        compile_expression(chunk, vm, *exp.data.exp);
        emit_byte(chunk, OP_NEGATE);
    } else if (exp.kind == STRING) {
        double value = table_get(&vm->globals, exp.data.sval);
        if (value == -1) {
            int index = add_string(vm, exp.data.sval);
            emit_bytes(chunk, 2, OP_STR_CONST, index);
        } else {
            emit_bytes(chunk, 2, OP_GET_GLOBAL, value);
        }
    } else {
        compile_expression(chunk, vm, exp.data.binexp->lhs);
        compile_expression(chunk, vm, exp.data.binexp->rhs);

        switch (exp.kind) {
            case ADD: emit_byte(chunk, OP_ADD); break;
            case SUB: emit_byte(chunk, OP_SUB); break;
            case MUL: emit_byte(chunk, OP_MUL); break;
            case DIV: emit_byte(chunk, OP_DIV); break;
            default: break;
        }
    }
}

static void print_chunk(BytecodeChunk *chunk) {
    for (uint8_t i = 0; i < chunk->code.count; i++) {
        switch (chunk->code.data[i]) {
            case OP_CONST: {
                printf("OP_CONST @ ");
                printf("%d\n", chunk->code.data[++i]);
                break;
            }
            case OP_STR_CONST: {
                printf("OP_STR_CONST @ ");
                printf("%d\n", chunk->code.data[++i]);
                break;
            }
            case OP_GET_GLOBAL: printf("OP_GET_GLOBAL\n"); break;
            case OP_SET_GLOBAL: printf("OP_SET_GLOBAL\n"); break;
            case OP_PRINT:      printf("OP_PRINT\n"); break;
            case OP_ADD:        printf("OP_ADD\n"); break;
            case OP_SUB:        printf("OP_SUB\n"); break;
            case OP_MUL:        printf("OP_MUL\n"); break;
            case OP_DIV:        printf("OP_DIV\n"); break;
            case OP_NEGATE:     printf("OP_NEGATE\n"); break;
            default:            printf("lolz\n"); break;
        }
    }
}

void compile(BytecodeChunk *chunk, VM *vm, Statement stmt) {
    switch (stmt.kind) {
        case STATEMENT_PRINT: {
            compile_expression(chunk, vm, stmt.exp);
            emit_byte(chunk, OP_PRINT);
            break;
        }
        case STATEMENT_LET: {
            compile_expression(chunk, vm, stmt.exp);
            emit_byte(chunk, OP_SET_GLOBAL);
            break;
        }
        default: break;
    }
#ifdef venom_debug
    print_chunk(chunk);
#endif
}