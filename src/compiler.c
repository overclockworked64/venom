#include <assert.h>
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
    memset(chunk, 0, sizeof(BytecodeChunk));
}

void free_chunk(BytecodeChunk *chunk) {
    dynarray_free(&chunk->code);
    for (int i = 0; i < chunk->sp_count; ++i) 
        free(chunk->sp[i]);
}

static uint8_t add_string(BytecodeChunk *chunk, const char *string) {
    /* check if the string is already present in the pool */
    for (uint8_t i = 0; i < chunk->sp_count; ++i) {
        /* if it is, return the index. */
        if (strcmp(chunk->sp[i], string) == 0) {
            return i;
        }
    }
    /* otherwise, insert the string into
     * the pool and return the index */
    char *s = own_string(string);
    chunk->sp[chunk->sp_count++] = s;
    return chunk->sp_count - 1;
}

static uint8_t add_constant(BytecodeChunk *chunk, double constant) {
    /* check if the constant is already present in the pool */
    for (uint8_t i = 0; i < chunk->cp_count; ++i) {
        /* if it is, return the index. */
        if (chunk->cp[i] == constant) {
            return i;
        }
    }
    /* otherwise, insert the constant into
     * the pool and return the index */
    chunk->cp[chunk->cp_count++] = constant;
    return chunk->cp_count - 1;
}

static int emit_byte(BytecodeChunk *chunk, uint8_t byte) {
    dynarray_insert(&chunk->code, byte);
    return chunk->code.count - 1;
}

static void emit_bytes(BytecodeChunk *chunk, uint8_t n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        uint8_t byte = va_arg(ap, int);
        emit_byte(chunk, byte);
    }
    va_end(ap);
}

static void compile_expression(BytecodeChunk *chunk, Expression exp, int *bytes_emitted) {
    switch (exp.kind) {
        case EXP_LITERAL: {
            uint8_t const_index = add_constant(chunk, exp.data.dval);
            emit_bytes(chunk, 2, OP_CONST, const_index);
            *bytes_emitted += 2;
            break;
        }
        case EXP_VARIABLE: {
            uint8_t name_index = add_string(chunk, exp.name);
            emit_bytes(chunk, 2, OP_GET_GLOBAL, name_index);
            *bytes_emitted += 2;
            break;
        }
        case EXP_UNARY: {
            compile_expression(chunk, *exp.data.exp, bytes_emitted);
            emit_byte(chunk, OP_NEGATE);
            *bytes_emitted += 1;
            break;
        }
        case EXP_BINARY: {
            compile_expression(chunk, exp.data.binexp->lhs, bytes_emitted);
            compile_expression(chunk, exp.data.binexp->rhs, bytes_emitted);

            if (strcmp(exp.operator, "+") == 0) {
                emit_byte(chunk, OP_ADD);
                *bytes_emitted += 1;
            } else if (strcmp(exp.operator, "-") == 0) {
                emit_byte(chunk, OP_SUB);
                *bytes_emitted += 1;
            } else if (strcmp(exp.operator, "*") == 0) {
                emit_byte(chunk, OP_MUL);
                *bytes_emitted += 1;
            } else if (strcmp(exp.operator, "/") == 0) {
                emit_byte(chunk, OP_DIV);
                *bytes_emitted += 1;
            } else if (strcmp(exp.operator, ">") == 0) {
                emit_byte(chunk, OP_GT);
                *bytes_emitted += 1;
            } else if (strcmp(exp.operator, "<") == 0) {
                emit_byte(chunk, OP_LT);
                *bytes_emitted += 1;
            } else if (strcmp(exp.operator, ">=") == 0) {
                emit_bytes(chunk, 2, OP_LT, OP_NOT);
                *bytes_emitted += 2;
            } else if (strcmp(exp.operator, "<=") == 0) {
                emit_bytes(chunk, 2, OP_GT, OP_NOT);
                *bytes_emitted += 2;
            } else if (strcmp(exp.operator, "==") == 0) {
                emit_byte(chunk, OP_EQ);
                *bytes_emitted += 1;
            } else if (strcmp(exp.operator, "!=") == 0) {
                emit_bytes(chunk, 2, OP_EQ, OP_NOT);
                *bytes_emitted += 2;
            } 

            break;
        }
        default: assert(0);
    }
}

#ifdef venom_debug
void disassemble(BytecodeChunk *chunk) {
    for (
        uint8_t *ip = chunk->code.data;
        ip < &chunk->code.data[chunk->code.count];  /* ip < addr of just beyond the last instruction */
        ip++
    ) {
        switch (*ip) {
            case OP_CONST: {
                uint8_t const_index = *++ip;
                printf("OP_CONST @ ");
                printf("%d ", chunk->code.data[const_index]);
                printf("('%.2f')\n", chunk->cp[const_index]);
                break;
            }
            case OP_STR_CONST: {
                uint8_t name_index = *++ip;
                printf("OP_STR_CONST @ ");
                printf("%d ", chunk->code.data[name_index]);
                printf("('%s')\n", chunk->sp[name_index]);
                break;
            }
            case OP_GET_GLOBAL: {
                uint8_t index = *++ip;
                printf("OP_GET_GLOBAL @ ");
                printf("%d\n", chunk->code.data[index]);
                break;
            }
            case OP_SET_GLOBAL: printf("OP_SET_GLOBAL\n"); break;
            case OP_ADD: printf("OP_ADD\n"); break;
            case OP_SUB: printf("OP_SUB\n"); break;
            case OP_MUL: printf("OP_MUL\n"); break;
            case OP_DIV: printf("OP_DIV\n"); break;
            case OP_EQ: printf("OP_EQ\n"); break;
            case OP_GT: printf("OP_GT\n"); break;
            case OP_LT: printf("OP_LT\n"); break;
            case OP_JNE: {
                printf("OP_JNE\n");
                ip += 2;
                break;
            }
            case OP_JMP: {
                printf("OP_JMP\n");
                ip += 2;
                break;
            }
            case OP_NOT: printf("OP_NOT\n"); break;
            case OP_NEGATE: printf("OP_NEGATE\n"); break;
            case OP_PRINT: printf("OP_PRINT\n"); break;
            default: printf("Unknown instruction.\n"); break;
        }
    }
}
#endif

static void patch_jump(BytecodeChunk *chunk, int jump_addr, int bytes_emitted) {
    chunk->code.data[jump_addr + 1] = (bytes_emitted >> 8) & 0xFF;
    chunk->code.data[jump_addr + 2] = bytes_emitted & 0xFF;
}

int compile(BytecodeChunk *chunk, Statement stmt) {
    int bytes_emitted = 0;
    switch (stmt.kind) {
        case STMT_PRINT: {
            compile_expression(chunk, stmt.exp, &bytes_emitted);
            emit_byte(chunk, OP_PRINT);
            bytes_emitted += 1;
            break;
        }
        case STMT_LET:
        case STMT_ASSIGN: {
            uint8_t name_index = add_string(chunk, stmt.name);
            emit_bytes(chunk, 2, OP_STR_CONST, name_index);
            compile_expression(chunk, stmt.exp, &bytes_emitted);
            emit_byte(chunk, OP_SET_GLOBAL);
            bytes_emitted += 3;
            break;
        }
        case STMT_BLOCK: {
            for (int i = 0; i < stmt.stmts.count; ++i) {
                bytes_emitted += compile(chunk, stmt.stmts.data[i]);
            }
            break;
        }
        case STMT_IF: {
            compile_expression(chunk, stmt.exp, &bytes_emitted); 

            int then_jmp = emit_byte(chunk, OP_JNE);
            emit_bytes(chunk, 2, 0xFF, 0xFF);
            int then_emitted = compile(chunk, *stmt.then_branch);
            patch_jump(chunk, then_jmp, then_emitted);

            if (stmt.else_branch != NULL) {
                int else_jmp = emit_byte(chunk, OP_JMP);
                emit_bytes(chunk, 2, 0xFF, 0xFF);
                int else_emitted = compile(chunk, *stmt.else_branch);
                patch_jump(chunk, else_jmp, else_emitted);
            }

            break;
        }
        default: assert(0);
    }
    return bytes_emitted;
}