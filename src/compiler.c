#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void init_chunk(BytecodeChunk *chunk) {
    chunk->code = malloc(sizeof(int) * 255);
    chunk->count = 0;
    chunk->ip = chunk->code;
}

void emit_const(int constant) {
    compiling_chunk.code[compiling_chunk.count++] = constant;
}

void emit_op(Opcode op) {
    compiling_chunk.code[compiling_chunk.count++] = op;
}

void compile_expression(BinaryExpression *exp, ExpressionKind kind) {
    if (exp->lhs.kind != LITERAL) compile_expression(exp->lhs.data.binexp, exp->lhs.kind);
    if (exp->rhs.kind != LITERAL) compile_expression(exp->rhs.data.binexp, exp->rhs.kind);

    if (exp->lhs.kind == LITERAL) {
        emit_op(OP_CONST);
        emit_const(exp->lhs.data.intval);
    }
    
    if (exp->rhs.kind == LITERAL) {
        emit_op(OP_CONST);
        emit_const(exp->rhs.data.intval);
    }

    switch (kind) {
        case ADD: emit_op(OP_ADD); break;
        case SUB: emit_op(OP_SUB); break;
        case MUL: emit_op(OP_MUL); break;
        case DIV: emit_op(OP_DIV); break;
        default: break;
    }
}

void compile(Statement stmt) {
    init_chunk(&compiling_chunk);
    switch (stmt.kind) {
        case STATEMENT_PRINT: {
            compile_expression(stmt.exp.data.binexp, stmt.exp.kind);
            emit_op(OP_PRINT);
        }
        default: break;
    }
}