#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "compiler.h"
#include "vm.h"

void init_chunk(BytecodeChunk *chunk) {
    chunk->code = malloc(sizeof(chunk->code[0]) * 255);
    chunk->count = 0;
    chunk->ip = chunk->code;
}

void free_chunk(BytecodeChunk *chunk) {
    free(chunk->code);
}

int add_constant(VM *vm, double constant) {
    vm->cp[vm->cpp++] = constant;
    return vm->cpp - 1;
}

void emit_byte(BytecodeChunk *chunk, uint8_t byte) {
    chunk->code[chunk->count++] = byte;
}

void emit_bytes(BytecodeChunk *chunk, int n, ...) {
    va_list ap;
    va_start(ap, n);

    for (int i = 0; i < n; ++i) {
        uint8_t byte = va_arg(ap, int);
        emit_byte(chunk, (uint8_t)byte);
    }

    va_end(ap);
}

void compile_expression(BytecodeChunk *chunk, VM *vm, const BinaryExpression *exp, ExpressionKind kind) {
    if (exp->lhs.kind != LITERAL) {
        compile_expression(chunk, vm, exp->lhs.data.binexp, exp->lhs.kind);
    } else {
        int index = add_constant(vm, exp->lhs.data.val);
        emit_bytes(chunk, 2, OP_CONST, index);
    }
    
    if (exp->rhs.kind != LITERAL) {
        compile_expression(chunk, vm, exp->rhs.data.binexp, exp->rhs.kind);
    } else {
        int index = add_constant(vm, exp->rhs.data.val);
        emit_bytes(chunk, 2, OP_CONST, index);
    }

    switch (kind) {
        case ADD: emit_byte(chunk, OP_ADD); break;
        case SUB: emit_byte(chunk, OP_SUB); break;
        case MUL: emit_byte(chunk, OP_MUL); break;
        case DIV: emit_byte(chunk, OP_DIV); break;
        default: break;
    }
}

void compile(BytecodeChunk *chunk, VM *vm, Statement stmt) {
    switch (stmt.kind) {
        case STATEMENT_PRINT: {
            compile_expression(chunk, vm, stmt.exp.data.binexp, stmt.exp.kind);
            emit_byte(chunk, OP_PRINT);
        }
        default: break;
    }
}