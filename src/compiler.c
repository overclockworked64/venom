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

static void emit_byte(BytecodeChunk *chunk, uint8_t byte) {
    dynarray_insert(&chunk->code, byte);
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

static int emit_jump(BytecodeChunk *chunk, Opcode jump) {
    emit_byte(chunk, jump);
    emit_bytes(chunk, 2, 0xFF, 0xFF);
    /* In this case, the jump is the last emitted instruction. 
     *
     * For example, if we have: [
     *     OP_CONST, operand,
     *     OP_CONST, operand,
     *     OP_EQ, 
     *     OP_JZ, operand, operand
     * ]                             ^-- count
     *
     * the count will be 8. Because the indexing is zero-based,
     * the count points to just beyond the two operands, so to
     * get to OP_JZ, we need to subtract 3 (two operands plus
     * one 'extra' slot to adjust for zero-based indexing). */
    return chunk->code.count - 3;
}

static void patch_jump(BytecodeChunk *chunk, int jump) {
    /* In this case, one or both branches have been compiled.
     *
     * For example, if we have: [
     *     OP_CONST, operand,
     *     OP_CONST, operand,
     *     OP_EQ, 
     *     OP_JZ, operand, operand,
     *     OP_CONST, operand,
     *     OP_PRINT,
     * ]             ^-- count
     *
     * We first adjust for zero-based indexing by subtracting 1
     * (such that count points to the last element. Then we take
     * the index of OP_JZ (5 in this case) and add 2 because we
     * need to adjust for the operands. The result of subtraction
     * of these two is the number of emitted bytes after the jump,
     * and we use that number to build a 16-bit offset that we use
     * to patch the jump. */
    int16_t bytes_emitted = (chunk->code.count - 1) - (jump + 2);
    chunk->code.data[jump+1] = (bytes_emitted >> 8) & 0xFF;
    chunk->code.data[jump+2] = bytes_emitted & 0xFF;
}

static void emit_loop(BytecodeChunk *chunk, int loop_start) {
    /* In this case, the conditional expression has been compiled.
     *
     * For example, if we have: [
     *     OP_STR_CONST @ 14 ('x')
     *     OP_CONST @ 14 ('0.00')
     *     OP_SET_GLOBAL
     *     OP_GET_GLOBAL @ 14
     *     OP_CONST @ 0 ('10.00')
     *     OP_LT
     *     OP_JZ
     *     OP_GET_GLOBAL @ 14
     *     OP_PRINT
     *     OP_STR_CONST @ 14 ('x')
     *     OP_GET_GLOBAL @ 14
     *     OP_CONST @ 13 ('1.00')
     *     OP_ADD
     *     OP_SET_GLOBAL
     *     OP_LOOP
     * ]            ^-- count
     *
     * In this case, the loop_start is '5'. If the count points to
     * one instruction beyond the end of the bytecode '23'), in order
     * to get to the beginning of the loop, we need to go backwards 18
     * instructions (chunk->code.count - loop_start = 23 - 5 = 18).
     * However, we need to make sure we include the 2-byte operand for
     * OP_LOOP, so we add +2 to the offset.
     */
    emit_byte(chunk, OP_JMP);
    int16_t offset = -(chunk->code.count - loop_start + 2);
    emit_byte(chunk, (offset >> 8) & 0xFF);
    emit_byte(chunk, offset & 0xFF);
}

static void compile_expression(BytecodeChunk *chunk, Expression exp) {
    switch (exp.kind) {
        case EXP_LITERAL: {
            uint8_t const_index = add_constant(chunk, exp.data.dval);
            emit_bytes(chunk, 2, OP_CONST, const_index);
            break;
        }
        case EXP_VARIABLE: {
            uint8_t name_index = add_string(chunk, exp.name);
            emit_bytes(chunk, 2, OP_GET_GLOBAL, name_index);
            break;
        }
        case EXP_UNARY: {
            compile_expression(chunk, *exp.data.exp);
            emit_byte(chunk, OP_NEGATE);
            break;
        }
        case EXP_BINARY: {
            compile_expression(chunk, exp.data.binexp->lhs);
            compile_expression(chunk, exp.data.binexp->rhs);

            if (strcmp(exp.operator, "+") == 0) {
                emit_byte(chunk, OP_ADD);
            } else if (strcmp(exp.operator, "-") == 0) {
                emit_byte(chunk, OP_SUB);
            } else if (strcmp(exp.operator, "*") == 0) {
                emit_byte(chunk, OP_MUL);
            } else if (strcmp(exp.operator, "/") == 0) {
                emit_byte(chunk, OP_DIV);
            } else if (strcmp(exp.operator, ">") == 0) {
                emit_byte(chunk, OP_GT);
            } else if (strcmp(exp.operator, "<") == 0) {
                emit_byte(chunk, OP_LT);
            } else if (strcmp(exp.operator, ">=") == 0) {
                emit_bytes(chunk, 2, OP_LT, OP_NOT);
            } else if (strcmp(exp.operator, "<=") == 0) {
                emit_bytes(chunk, 2, OP_GT, OP_NOT);
            } else if (strcmp(exp.operator, "==") == 0) {
                emit_byte(chunk, OP_EQ);
            } else if (strcmp(exp.operator, "!=") == 0) {
                emit_bytes(chunk, 2, OP_EQ, OP_NOT);
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
            case OP_JZ: {
                printf("OP_JZ\n");
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

void compile(BytecodeChunk *chunk, Statement stmt) {
    switch (stmt.kind) {
        case STMT_PRINT: {
            compile_expression(chunk, stmt.exp);
            emit_byte(chunk, OP_PRINT);
            break;
        }
        case STMT_LET:
        case STMT_ASSIGN: {
            uint8_t name_index = add_string(chunk, stmt.name);
            emit_bytes(chunk, 2, OP_STR_CONST, name_index);
            compile_expression(chunk, stmt.exp);
            emit_byte(chunk, OP_SET_GLOBAL);
            break;
        }
        case STMT_BLOCK: {
            for (int i = 0; i < stmt.stmts.count; ++i) {
                compile(chunk, stmt.stmts.data[i]);
            }
            break;
        }
        case STMT_IF: {
            /* We first compile the conditional expression because the VM
            .* expects something like OP_EQ to have already been executed
             * and a boolean placed on the stack by the time it encounters
             * an instruction like OP_JZ. */
            compile_expression(chunk, stmt.exp);
 
            /* Then, we emit an OP_JZ which jumps to the else clause if the
             * condition is falsey. Because we do not know the size of the
             * bytecode in the 'then' branch ahead of time, we do backpatching:
             * first, we emit 0xFFFF as the relative jump offset which acts as
             * a placeholder for the real jump offset that will be known only
             * after we compile the 'then' branch because at that point the
             * size of the 'then' branch is known. */ 
            int then_jump = emit_jump(chunk, OP_JZ);
            compile(chunk, *stmt.then_branch);

            /* Then, we emit OP_JMP, patch the 'then' jump,
             * and compile the 'else' branch. */
            int else_jump = emit_jump(chunk, OP_JMP);
            patch_jump(chunk, then_jump);

            if (stmt.else_branch != NULL) {
                compile(chunk, *stmt.else_branch);
            }

            /* Finally, we patch the 'else' jump. If the 'else' branch
             + wasn't compiled, the offset should be zeroed out. */
            patch_jump(chunk, else_jump);

            break;
        }
        case STMT_WHILE: {
            /* We need to mark the beginning of the loop before we compile
             * the conditional expression, so that we can emit OP_LOOP later. */
            int loop_start = chunk->code.count;

            /* We then compile the conditional expression because the VM
            .* expects something like OP_EQ to have already been executed
             * and a boolean placed on the stack by the time it encounters
             * an instruction like OP_JZ. */
            compile_expression(chunk, stmt.exp);
            
            /* Then, we emit an OP_JZ which jumps to the else clause if the
             * condition is falsey. Because we do not know the size of the
             * bytecode in the 'then' branch ahead of time, we do backpatching:
             * first, we emit 0xFFFF as the relative jump offset which acts as
             * a placeholder for the real jump offset that will be known only
             * after we compile the 'then' branch because at that point the
             * size of the 'then' branch is known. */ 
            int exit_jump = emit_jump(chunk, OP_JZ);
            compile(chunk, *stmt.body);

            /* Then, we emit OP_JMP with a negative offset. */
            emit_loop(chunk, loop_start);

            /* Finally, we patch the jump. */
            patch_jump(chunk, exit_jump);

            break;
        }
        default: assert(0);
    }
}