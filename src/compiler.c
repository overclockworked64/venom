#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "compiler.h"
#include "vm.h"
#include "util.h"

#define venom_debug

void init_chunk(BytecodeChunk *chunk) {
    dynarray_init(&chunk->code);
    chunk->sp_count = 0;
    chunk->cp_count = 0;
}

void free_chunk(BytecodeChunk *chunk) {
    dynarray_free(&chunk->code);
    for (int i = 0; i < chunk->sp_count; ++i) 
        free(chunk->sp[i]);
}

uint8_t add_string(BytecodeChunk *chunk, const char *string) {
    char *s = own_string(string);
    chunk->sp[chunk->sp_count++] = s;
    return chunk->sp_count - 1;
}

uint8_t add_constant(BytecodeChunk *chunk, double constant) {
    chunk->cp[chunk->cp_count++] = constant;
    return chunk->cp_count - 1;
}

void emit_byte(BytecodeChunk *chunk, uint8_t byte) {
    dynarray_insert(&chunk->code, byte);
}

void emit_bytes(BytecodeChunk *chunk, uint8_t n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        uint8_t byte = va_arg(ap, int);
        emit_byte(chunk, byte);
    }
    va_end(ap);
}

void compile_expression(BytecodeChunk *chunk, Expression exp) {
    if (exp.kind == LITERAL) {
        uint8_t index = add_constant(chunk, exp.data.dval);
        emit_bytes(chunk, 2, OP_CONST, index);
    } else if (exp.kind == VARIABLE) {
        uint8_t name_index = add_string(chunk, exp.name);
        emit_bytes(chunk, 2, OP_GET_GLOBAL, name_index);
    } else if (exp.kind == UNARY) {
        compile_expression(chunk, *exp.data.exp);
        emit_byte(chunk, OP_NEGATE);
    } else {
        compile_expression(chunk, exp.data.binexp->lhs);
        compile_expression(chunk, exp.data.binexp->rhs);
 
        switch (*exp.operator) {
            case '+': emit_byte(chunk, OP_ADD); break;
            case '-': emit_byte(chunk, OP_SUB); break;
            case '*': emit_byte(chunk, OP_MUL); break;
            case '/': emit_byte(chunk, OP_DIV); break;
            default: break;
        }
    }
}

#ifdef venom_debug
static void print_chunk(BytecodeChunk *chunk) {
    for (uint8_t i = 0; i < chunk->code.count; i++) {
        switch (chunk->code.data[i]) {
            case OP_CONST: printf("OP_CONST @ %d\n", chunk->code.data[++i]); break;
            case OP_STR_CONST: printf("OP_STR_CONST @ %d\n", chunk->code.data[++i]); break;
            case OP_GET_GLOBAL: printf("OP_GET_GLOBAL @ %d\n", chunk->code.data[++i]); break;
            case OP_SET_GLOBAL: printf("OP_SET_GLOBAL\n"); break;
            case OP_PRINT: printf("OP_PRINT\n"); break;
            case OP_ADD: printf("OP_ADD\n"); break;
            case OP_SUB: printf("OP_SUB\n"); break;
            case OP_MUL: printf("OP_MUL\n"); break;
            case OP_DIV: printf("OP_DIV\n"); break;
            case OP_NEGATE: printf("OP_NEGATE\n"); break;
            default: printf("Unknown instruction.\n"); break;
        }
    }
}
#endif

void compile(BytecodeChunk *chunk, Statement stmt) {
    switch (stmt.kind) {
        case STATEMENT_PRINT: {
            compile_expression(chunk, stmt.exp);
            emit_byte(chunk, OP_PRINT);
            break;
        }
        case STATEMENT_LET: {
            uint8_t name_index = add_string(chunk, stmt.name);
            emit_bytes(chunk, 2, OP_STR_CONST, name_index);
            compile_expression(chunk, stmt.exp);
            emit_byte(chunk, OP_SET_GLOBAL);
            break;
        }
        default: break;
    }
#ifdef venom_debug
    print_chunk(chunk);
#endif
}