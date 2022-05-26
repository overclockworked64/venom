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

void init_compiler(Compiler *compiler) {
    memset(compiler, 0, sizeof(Compiler));
}

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
     *     OP_JMP
     * ]            ^-- count
     *
     * In this case, the loop_start is at '5'. If the count points to
     * one instruction beyond the end of the bytecode '23'), in order
     * to get to the beginning of the loop, we need to go backwards 18
     * instructions (chunk->code.count - loop_start = 23 - 5 = 18).
     * However, we need to make sure we include the 2-byte operand for
     * OP_JMP, so we add +2 to the offset.
     */
    emit_byte(chunk, OP_JMP);
    int16_t offset = -(chunk->code.count - loop_start + 2);
    emit_byte(chunk, (offset >> 8) & 0xFF);
    emit_byte(chunk, offset & 0xFF);
}

static int var_index(String_DynArray vars, char *name) {
    for (size_t i = 0; i < vars.count; ++i) {
        if (strcmp(vars.data[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static void compile_expression(
    Compiler *compiler,
    BytecodeChunk *chunk,
    Expression exp,
    bool scoped
) {
    switch (exp.kind) {
        case EXP_LITERAL: {
            compiler->stack_size++;
            uint8_t const_index = add_constant(chunk, exp.data.dval);
            emit_bytes(chunk, 2, OP_CONST, const_index);
            break;
        }
        case EXP_VARIABLE: {
            compiler->stack_size++;
            uint8_t name_index = add_string(chunk, exp.name);
            if (!scoped) {
                emit_bytes(chunk, 2, OP_GET_GLOBAL, name_index);
            } else {
                int index = var_index(compiler->locals, exp.name);
                emit_bytes(chunk, 2, OP_GET_LOCAL, compiler->stack_size + compiler->paramcount - index);
                printf("compiler->stack_size is: %d\n", compiler->stack_size);
                printf("index is: %d\n", index);
                printf("compiler->paramcount is: %d\n", compiler->paramcount);
            }
            break;
        }
        case EXP_UNARY: {
            compile_expression(compiler, chunk, *exp.data.exp, scoped);
            emit_byte(chunk, OP_NEGATE);
            break;
        }
        case EXP_BINARY: {
            compile_expression(compiler, chunk, exp.data.binexp->lhs, scoped);
            compile_expression(compiler, chunk, exp.data.binexp->rhs, scoped);

            if (strcmp(exp.operator, "+") == 0) {
                emit_byte(chunk, OP_ADD);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, "-") == 0) {
                emit_byte(chunk, OP_SUB);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, "*") == 0) {
                emit_byte(chunk, OP_MUL);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, "/") == 0) {
                emit_byte(chunk, OP_DIV);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, ">") == 0) {
                emit_byte(chunk, OP_GT);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, "<") == 0) {
                emit_byte(chunk, OP_LT);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, ">=") == 0) {
                emit_bytes(chunk, 2, OP_LT, OP_NOT);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, "<=") == 0) {
                emit_bytes(chunk, 2, OP_GT, OP_NOT);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, "==") == 0) {
                emit_byte(chunk, OP_EQ);
                compiler->stack_size--;
            } else if (strcmp(exp.operator, "!=") == 0) {
                emit_bytes(chunk, 2, OP_EQ, OP_NOT);
                compiler->stack_size--;
            }

            break;
        }
        case EXP_CALL: {
            for (size_t i = 0; i < exp.arguments.count; ++i) {
                compile_expression(compiler, chunk, exp.arguments.data[i], scoped);
            }
            uint8_t funcname_index = add_string(chunk, exp.name);
            emit_bytes(chunk, 2, OP_INVOKE, funcname_index);
            compiler->stack_size++;
            emit_bytes(chunk, 2, OP_SET_LOCAL, compiler->paramcount);
            compiler->stack_size--;
            for (size_t i = 0; i < compiler->paramcount - 1; ++i) {
                emit_byte(chunk, OP_POP);
                compiler->stack_size--;
            }

            break;
        }
        default: assert(0);
    }
}

static void compile_expression_noemit(Compiler *compiler, BytecodeChunk *chunk, Expression exp, bool scoped) {
    switch (exp.kind) {
        case EXP_LITERAL: {
            break;
        }
        case EXP_VARIABLE: {
            dynarray_insert(&compiler->locals, exp.name);
            break;
        }
        case EXP_UNARY: {
            compile_expression_noemit(compiler, chunk, *exp.data.exp, scoped);
            break;
        }
        case EXP_BINARY: {
            compile_expression_noemit(compiler, chunk, exp.data.binexp->lhs, scoped);
            compile_expression_noemit(compiler, chunk, exp.data.binexp->rhs, scoped);
            break;
        }
        case EXP_CALL: {
            for (size_t i = 0; i < exp.arguments.count; ++i) {
                compile_expression_noemit(compiler, chunk, exp.arguments.data[i], scoped);
            }
            break;
        }
        default: assert(0);
    }
}

void first_pass(Compiler *compiler, BytecodeChunk *chunk, Statement stmt, bool scoped) {
    switch (stmt.kind) {
        case STMT_PRINT: {
            compile_expression_noemit(compiler, chunk, stmt.exp, scoped);
            break;
        }
        case STMT_LET:
        case STMT_ASSIGN: {
            compile_expression_noemit(compiler, chunk, stmt.exp, scoped);
            if (!scoped) {
            } else {
                dynarray_insert(&compiler->locals, stmt.name);
                int index = var_index(compiler->locals, stmt.name);
            }
            break;
        }
        case STMT_BLOCK: {
            for (size_t i = 0; i < stmt.stmts.count; ++i) {
                first_pass(compiler, chunk, stmt.stmts.data[i], scoped);
            }
            break;
        }
        case STMT_IF: {
            compile_expression_noemit(compiler, chunk, stmt.exp, scoped);

            first_pass(compiler, chunk, *stmt.then_branch, scoped);

            if (stmt.else_branch != NULL) {
                first_pass(compiler, chunk, *stmt.else_branch, scoped);
            }

            break;
        }
        case STMT_WHILE: {    
            compile_expression_noemit(compiler, chunk, stmt.exp, scoped);
            first_pass(compiler, chunk, *stmt.body, scoped);
            break;
        }
        case STMT_FN: {
            for (size_t i = 0; i < stmt.stmts.count; ++i) {
                first_pass(compiler, chunk, stmt.stmts.data[i], true);
            }
            break;
        }
        case STMT_RETURN: { 
            compile_expression_noemit(compiler, chunk, stmt.exp, scoped);
            break;
        }
        default: assert(0);
    }
}

#ifdef venom_debug
void disassemble(BytecodeChunk *chunk) {
    int i = 0;
    for (
        uint8_t *ip = chunk->code.data;    
        ip < &chunk->code.data[chunk->code.count];  /* ip < addr of just beyond the last instruction */
        ip++, ++i
    ) {
        switch (*ip) {
            case OP_CONST: {
                uint8_t const_index = *++ip;
                printf("%d: ", i);
                printf("OP_CONST, byte (idx: %d): (val: '%f')\n", const_index, chunk->cp[const_index]);
                ++i;
                break;
            }
            case OP_STR_CONST: {
                uint8_t name_index = *++ip;
                printf("%d: ", i);
                printf("OP_STR_CONST, byte (idx: %d): (val: '%s')\n", name_index, chunk->sp[name_index]);
                ++i;
                break;
            }
            case OP_GET_GLOBAL: {
                printf("%d: ", i);
                printf("OP_GET_GLOBAL + byte\n");
                ++ip;
                ++i;
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t name_index = *++ip;
                printf("%d: ", i);
                printf("OP_GET_LOCAL, byte (%d): ('%s')\n", name_index, chunk->sp[name_index]);
                ++i;
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t index = *++ip;
                printf("%d: ", i);
                printf("OP_SET_LOCAL, byte (%d)\n", index);
                ++i;
                break;
            }
            case OP_SET_GLOBAL: {
                printf("%d: ", i);
                printf("OP_SET_GLOBAL\n");
                break;
            }
            case OP_ADD: {
                printf("%d: ", i);
                printf("OP_ADD\n");
                break;
            }
            case OP_SUB: {
                printf("%d: ", i);
                printf("OP_SUB\n");
                break;
            }
            case OP_MUL: {
                printf("%d: ", i);
                printf("OP_MUL\n");
                break;
            }
            case OP_DIV: {
                printf("%d: ", i);
                printf("OP_DIV\n");
                break;
            }
            case OP_EQ: {
                printf("%d: ", i);
                printf("OP_EQ\n");
                break;
            }
            case OP_GT: {
                printf("%d: ", i);
                printf("OP_GT\n");
                break;
            }
            case OP_LT: {
                printf("%d: ", i);
                printf("OP_LT\n");
                break;
            }
            case OP_JZ: {
                printf("%d: ", i);
                int16_t offset = *++ip;
                offset <<= 8;
                offset |= *++ip;
                printf("OP_JZ, byte, byte (offset: '%d')\n", offset);
                i += 2;
                break;
            }
            case OP_JMP: {
                printf("%d: ", i);
                int16_t offset = *++ip;
                offset <<= 8;
                offset |= *++ip;
                printf("OP_JMP, byte, byte (offset: '%d')\n", offset);
                i += 2;
                break;
            }
            case OP_FUNC: {
                printf("%d: ", i);
                printf("OP_FUNC ");
                uint8_t funcname_index = *++ip;
                printf(", byte (name: '%d' (%s)')", funcname_index, chunk->sp[funcname_index]);
                uint8_t paramcount = *++ip;
                printf(", byte (paramcount: '%d')", paramcount);
                for (; i < paramcount; ++i) {
                    uint8_t paramname_index = *++ip;
                    printf(", byte (param: '%d' (%s)')", paramname_index, chunk->sp[paramname_index]);
                }
                uint8_t location = *++ip;
                printf(", byte (location: '%d')\n", location);
                i++;
                break;
            }
            case OP_INVOKE: {
                uint8_t funcname_index = *++ip;
                char *funcname = chunk->sp[funcname_index];
                printf("%d: ", i);
                printf("OP_INVOKE, byte (funcname_index: '%d' ('%s')\n", funcname_index, funcname);
                break;
            }
            case OP_RET: {
                printf("%d: ", i);
                printf("OP_RET\n");
                break;
            }
            case OP_NOT: {
                printf("%d: ", i);
                printf("OP_NOT\n");
                break;
            }
            case OP_NEGATE: {
                printf("%d: ", i);
                printf("OP_NEGATE\n");
                break;
            }
            case OP_PRINT: {
                printf("%d: ", i);
                printf("OP_PRINT\n"); 
                break;
            }
            case OP_POP: {
                printf("%d: ", i);
                printf("OP_POP\n");
                break;
            }
            default: printf("Unknown instruction.\n"); break;
        }
    }
}
#endif

void compile(Compiler *compiler, BytecodeChunk *chunk, Statement stmt, bool scoped) {
    switch (stmt.kind) {
        case STMT_PRINT: {
            compile_expression(compiler, chunk, stmt.exp, scoped);
            emit_byte(chunk, OP_PRINT);
            compiler->stack_size--;
            break;
        }
        case STMT_LET:
        case STMT_ASSIGN: {
            compile_expression(compiler, chunk, stmt.exp, scoped);
            if (!scoped) {
                compiler->stack_size -= 2;
                emit_byte(chunk, OP_SET_GLOBAL);
            } else {
                compiler->stack_size--;
                int index = var_index(compiler->locals, stmt.name);
                emit_bytes(
                    chunk, 2,
                    OP_SET_LOCAL,
                    compiler->stack_size + compiler->paramcount - index + 1
                );
            }
            break;
        }
        case STMT_BLOCK: {
            for (size_t i = 0; i < stmt.stmts.count; ++i) {
                compile(compiler, chunk, stmt.stmts.data[i], scoped);
            }
            break;
        }
        case STMT_IF: {
            /* We first compile the conditional expression because the VM
            .* expects something like OP_EQ to have already been executed
             * and a boolean placed on the stack by the time it encounters
             * an instruction like OP_JZ. */
            compile_expression(compiler, chunk, stmt.exp, scoped);
 
            /* Then, we emit an OP_JZ which jumps to the else clause if the
             * condition is falsey. Because we do not know the size of the
             * bytecode in the 'then' branch ahead of time, we do backpatching:
             * first, we emit 0xFFFF as the relative jump offset which acts as
             * a placeholder for the real jump offset that will be known only
             * after we compile the 'then' branch because at that point the
             * size of the 'then' branch is known. */ 
            int then_jump = emit_jump(chunk, OP_JZ);
            compiler->stack_size--;
            compile(compiler, chunk, *stmt.then_branch, scoped);

            int else_jump = emit_jump(chunk, OP_JMP);

            /* Then, we patch the 'then' jump. */
            patch_jump(chunk, then_jump);

            if (stmt.else_branch != NULL) {
                compile(compiler, chunk, *stmt.else_branch, scoped);
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
            compile_expression(compiler, chunk, stmt.exp, scoped);
            
            /* Then, we emit an OP_JZ which jumps to the else clause if the
             * condition is falsey. Because we do not know the size of the
             * bytecode in the body of the 'while' loop ahead of time, we do
             * backpatching: first, we emit 0xFFFF as the relative jump offset
             * which acts as a placeholder for the real jump offset that will
             * be known only after we compile the body of the 'while' loop,
             * because at that point its size is known. */ 
            int exit_jump = emit_jump(chunk, OP_JZ);
            compiler->stack_size--;
            compile(compiler, chunk, *stmt.body, scoped);

            /* Then, we emit OP_JMP with a negative offset. */
            emit_loop(chunk, loop_start);

            /* Finally, we patch the jump. */
            patch_jump(chunk, exit_jump);

            break;
        }
        case STMT_FN: {           
            emit_byte(chunk, OP_FUNC);

            /* Emit function name. */
            uint8_t name_index = add_string(chunk, stmt.name);
            emit_byte(chunk, name_index);

            /* Emit parameter count. */
            emit_byte(chunk, (uint8_t)stmt.parameters.count);

            /* Emit parameter names. */
            for (size_t i = 0; i < stmt.parameters.count; ++i) {
                uint8_t parameter_index = add_string(chunk, stmt.parameters.data[i]);
                emit_byte(chunk, parameter_index);
            }
           
            /* Emit the location of the start of the function. */
            emit_byte(chunk, (uint8_t)chunk->code.count + 4);
            
            int jump = emit_jump(chunk, OP_JMP);

            compiler->paramcount = stmt.parameters.count;

            for (size_t i = 0; i < stmt.stmts.count; ++i) {
                compile(compiler, chunk, stmt.stmts.data[i], true);
            }

            patch_jump(chunk, jump);

            break;
        }
        case STMT_RETURN: { 
            compile_expression(compiler, chunk, stmt.exp, scoped);
            emit_byte(chunk, OP_RET);
            compiler->stack_size--;
            break;
        }
        default: assert(0);
    }
}