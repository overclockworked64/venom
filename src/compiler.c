#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void init_chunk() {
    chunk.code = malloc(sizeof(chunk.code[0]) * 255);
    chunk.count = 0;
    chunk.ip = chunk.code;
}

void free_chunk() {
    free(chunk.code);
}

void emit_const(int constant) {
    vm.cp[vm.cpp++] = constant;
    chunk.code[chunk.count++] = vm.cpp - 1;
}

void emit_op(Opcode op) {
    chunk.code[chunk.count++] = op;
}

void compile_expression(const BinaryExpression *exp, ExpressionKind kind) {
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
    init_chunk();
    switch (stmt.kind) {
        case STATEMENT_PRINT: {
            compile_expression(stmt.exp.data.binexp, stmt.exp.kind);
            emit_op(OP_PRINT);
        }
        default: break;
    }
    emit_op(OP_EXIT);
}