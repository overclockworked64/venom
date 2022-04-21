#include <stdlib.h>
#include <stdio.h>
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

void emit_const(BytecodeChunk *chunk, VM *vm, double constant) {
    vm->cp[vm->cpp++] = constant;
    chunk->code[chunk->count++] = vm->cpp - 1;
}

void emit_op(BytecodeChunk *chunk, Opcode op) {
    chunk->code[chunk->count++] = op;
}

void compile_expression(BytecodeChunk *chunk, VM *vm, const BinaryExpression *exp, ExpressionKind kind) {
    if (exp->lhs.kind != LITERAL) {
        compile_expression(chunk, vm, exp->lhs.data.binexp, exp->lhs.kind);
    } else {
        emit_op(chunk, OP_CONST);
        emit_const(chunk, vm, exp->lhs.data.val);
    }
    
    if (exp->rhs.kind != LITERAL) {
        compile_expression(chunk, vm, exp->rhs.data.binexp, exp->rhs.kind);
    } else {
        emit_op(chunk, OP_CONST);
        emit_const(chunk, vm, exp->rhs.data.val);
    }

    switch (kind) {
        case ADD: emit_op(chunk, OP_ADD); break;
        case SUB: emit_op(chunk, OP_SUB); break;
        case MUL: emit_op(chunk, OP_MUL); break;
        case DIV: emit_op(chunk, OP_DIV); break;
        default: break;
    }
}

void compile(BytecodeChunk *chunk, VM *vm, Statement stmt) {
    switch (stmt.kind) {
        case STATEMENT_PRINT: {
            compile_expression(chunk, vm, stmt.exp.data.binexp, stmt.exp.kind);
            emit_op(chunk, OP_PRINT);
        }
        default: break;
    }
}