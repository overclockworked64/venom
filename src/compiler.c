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
    for (int i = 0; i < chunk->sp_count; i++) 
        free(chunk->sp[i]);
}

static uint8_t add_string(BytecodeChunk *chunk, const char *string) {
    /* Check if the string is already present in the pool. */
    for (uint8_t i = 0; i < chunk->sp_count; i++) {
        /* If it is, return the index. */
        if (strcmp(chunk->sp[i], string) == 0) {
            return i;
        }
    }
    /* Otherwise, own the string, insert it into the pool
     * and return the index. */
    char *s = own_string(string);
    chunk->sp[chunk->sp_count++] = s;
    return chunk->sp_count - 1;
}

static uint8_t add_constant(BytecodeChunk *chunk, double constant) {
    /* Check if the constant is already present in the pool. */
    for (uint8_t i = 0; i < chunk->cp_count; i++) {
        /* If it is, return the index. */
        if (chunk->cp[i] == constant) {
            return i;
        }
    }
    /* Otherwise, insert the constant into the pool
     * and return the index. */
    chunk->cp[chunk->cp_count++] = constant;
    return chunk->cp_count - 1;
}

static void emit_byte(BytecodeChunk *chunk, uint8_t byte) {
    dynarray_insert(&chunk->code, byte);
}

static void emit_bytes(BytecodeChunk *chunk, uint8_t n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; i++) {
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

static int resolve_local(Compiler *compiler, char *name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        if (strcmp(compiler->locals[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static void compile_expression(Compiler *compiler, BytecodeChunk *chunk, Expression exp) {
    switch (exp.kind) {
        case EXP_LITERAL: {
            if (exp.name == NULL) {
                uint8_t const_index = add_constant(chunk, exp.data.dval);
                emit_bytes(chunk, 2, OP_CONST, const_index);
                break;
            } else {
                if (strcmp(exp.name, "true") == 0) {
                    emit_byte(chunk, OP_TRUE);
                } else if (strcmp(exp.name, "false") == 0) {
                    emit_bytes(chunk, 2, OP_TRUE, OP_NOT);
                } else if (strcmp(exp.name, "null") == 0) {
                    emit_byte(chunk, OP_NULL);
                }
            }
            break;
        }
        case EXP_STRING: {
            uint8_t const_index = add_string(chunk, exp.data.str);
            emit_bytes(chunk, 2, OP_STR, const_index);
            break;
        }
        case EXP_VARIABLE: {
            int index = resolve_local(compiler, exp.name);
            if (index == -1) {
                uint8_t name_index = add_string(chunk, exp.name);
                emit_bytes(chunk, 2, OP_GET_GLOBAL, name_index);
            } else {
                emit_bytes(chunk, 2, OP_DEEP_GET, index);
            }
            break;
        }
        case EXP_UNARY: {
            compile_expression(compiler, chunk, *exp.data.exp);
            emit_byte(chunk, OP_NEGATE);
            break;
        }
        case EXP_BINARY: {
            compile_expression(compiler, chunk, exp.data.binexp->lhs);
            compile_expression(compiler, chunk, exp.data.binexp->rhs);

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
        case EXP_CALL: {
            for (size_t i = 0; i < exp.arguments.count; i++) {
                compile_expression(compiler, chunk, exp.arguments.data[i]);
            }
            uint8_t funcname_index = add_string(chunk, exp.name);
            emit_bytes(chunk, 3, OP_INVOKE, funcname_index, exp.arguments.count);
            break;
        }
        case EXP_ASSIGN: {
            compile_expression(compiler, chunk, exp.data.binexp->rhs);
            int index = resolve_local(compiler, exp.data.binexp->lhs.name);
            if (index != -1) {
                emit_bytes(chunk, 2, OP_DEEP_SET, index);
            } else {
                uint8_t name_index = add_string(chunk, exp.data.binexp->lhs.name);
                emit_bytes(chunk, 2, OP_SET_GLOBAL, name_index);
            }
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
        ip++, i++
    ) {
        switch (*ip) {
            case OP_CONST: {
                uint8_t const_index = *++ip;
                printf("%d: ", i);
                printf("OP_CONST, byte (idx: %d): (val: '%f')\n", const_index, chunk->cp[const_index]);
                i++;
                break;
            }
            case OP_GET_GLOBAL: {
                printf("%d: ", i);
                printf("OP_GET_GLOBAL + byte\n");
                ++ip;
                i++;
                break;
            }
            case OP_DEEP_GET: {
                uint8_t name_index = *++ip;
                printf("%d: ", i);
                printf("OP_DEEP_GET, byte (%d): ('%s')\n", name_index, chunk->sp[name_index]);
                i++;
                break;
            }
            case OP_DEEP_SET: {
                uint8_t index = *++ip;
                printf("%d: ", i);
                printf("OP_DEEP_SET, byte (%d)\n", index);
                i++;
                break;
            }
            case OP_SET_GLOBAL: {
                printf("%d: ", i);
                printf("OP_SET_GLOBAL\n");
                i++;
                ++ip;
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
                for (; i < paramcount; i++) {
                    uint8_t paramname_index = *++ip;
                    printf(", byte (param: '%d' (%s)')", paramname_index, chunk->sp[paramname_index]);
                }
                uint8_t location = *++ip;
                printf(", byte (location: '%d')\n", location);
                i += 2 + paramcount;
                break;
            }
            case OP_INVOKE: {
                uint8_t funcname_index = *++ip;
                char *funcname = chunk->sp[funcname_index];
                printf("%d: ", i);
                printf("OP_INVOKE, byte (funcname_index: '%d' ('%s')\n", funcname_index, funcname);
                uint8_t argcount = *++ip;
                printf(", byte (paramcount: '%d')", argcount);
                i += 2;
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
            case OP_NULL: {
                printf("%d: ", i);
                printf("OP_NULL\n");
                break;
            }
            default: printf("Unknown instruction: %d.\n", *ip); break;
        }
    }
}
#endif

void compile(Compiler *compiler, BytecodeChunk *chunk, Statement stmt, bool scoped) {
    switch (stmt.kind) {
        case STMT_PRINT: {
            compile_expression(compiler, chunk, stmt.as.stmt_print.exp);
            emit_byte(chunk, OP_PRINT);        
            break;
        }
        case STMT_LET: {
            compile_expression(compiler, chunk, stmt.as.stmt_let.initializer);
            uint8_t name_index = add_string(chunk, stmt.as.stmt_let.name);
            if (!scoped) {
                emit_bytes(chunk, 2, OP_SET_GLOBAL, name_index);
            } else {
                compiler->locals[compiler->locals_count++] = chunk->sp[name_index];
            }
            break;
        }
        case STMT_EXPR: {
            compile_expression(compiler, chunk, stmt.as.stmt_expr.exp);
            break;
        }
        case STMT_BLOCK: {
            for (size_t i = 0; i < stmt.as.stmt_block.stmts.count; i++) {
                compile(compiler, chunk, stmt.as.stmt_block.stmts.data[i], scoped);
            }
            break;
        }
        case STMT_IF: {
            /* We first compile the conditional expression because the VM
            .* expects something like OP_EQ to have already been executed
             * and a boolean placed on the stack by the time it encounters
             * an instruction like OP_JZ. */
            compile_expression(compiler, chunk, stmt.as.stmt_if.condition);
 
            /* Then, we emit an OP_JZ which jumps to the else clause if the
             * condition is falsey. Because we do not know the size of the
             * bytecode in the 'then' branch ahead of time, we do backpatching:
             * first, we emit 0xFFFF as the relative jump offset which acts as
             * a placeholder for the real jump offset that will be known only
             * after we compile the 'then' branch because at that point the
             * size of the 'then' branch is known. */ 
            int then_jump = emit_jump(chunk, OP_JZ);
            
            compile(compiler, chunk, *stmt.as.stmt_if.then_branch, scoped);

            int else_jump = emit_jump(chunk, OP_JMP);

            /* Then, we patch the 'then' jump. */
            patch_jump(chunk, then_jump);

            if (stmt.as.stmt_if.else_branch != NULL) {
                compile(compiler, chunk, *stmt.as.stmt_if.else_branch, scoped);
            }

            /* Finally, we patch the 'else' jump. If the 'else' branch
            + wasn't compiled, the offset should be zeroed out. */
            patch_jump(chunk, else_jump);

            break;
        }
        case STMT_WHILE: {    
            /* We need to mark the beginning of the loop before we compile
             * the conditional expression, so that we know where to return
             * after the body of the loop is executed. */
            int loop_start = chunk->code.count;

            /* We then compile the conditional expression because the VM
            .* expects something like OP_EQ to have already been executed
             * and a boolean placed on the stack by the time it encounters
             * an instruction like OP_JZ. */
            compile_expression(compiler, chunk, stmt.as.stmt_while.condition);
            
            /* Then, we emit an OP_JZ which jumps to the else clause if the
             * condition is falsey. Because we do not know the size of the
             * bytecode in the body of the 'while' loop ahead of time, we do
             * backpatching: first, we emit 0xFFFF as the relative jump offset
             * which acts as a placeholder for the real jump offset that will
             * be known only after we compile the body of the 'while' loop,
             * because at that point its size is known. */ 
            int exit_jump = emit_jump(chunk, OP_JZ);
            
            /* Then, we compile the body of the loop. */
            compile(compiler, chunk, *stmt.as.stmt_while.body, scoped);

            /* Then, we emit OP_JMP with a negative offset. */
            emit_loop(chunk, loop_start);

            /* Finally, we patch the jump. */
            patch_jump(chunk, exit_jump);

            break;
        }
        case STMT_FN: {           
            init_compiler(compiler);

            emit_byte(chunk, OP_FUNC);

            /* Emit function name. */
            uint8_t name_index = add_string(chunk, stmt.as.stmt_fn.name);
            emit_byte(chunk, name_index);

            /* Emit parameter count. */
            emit_byte(chunk, (uint8_t)stmt.as.stmt_fn.parameters.count);

            /* Add parameter names to compiler->locals. */
            for (size_t i = 0; i < stmt.as.stmt_fn.parameters.count; i++) {
                uint8_t parameter_index = add_string(chunk, stmt.as.stmt_fn.parameters.data[i]);
                compiler->locals[compiler->locals_count++] = chunk->sp[parameter_index];
            }
           
            /* Emit the location of the start of the function. */
            emit_byte(chunk, (uint8_t)(chunk->code.count + 4));
            
            /* Emit the jump because we don't want to execute
             * the code the first time we encounter it. */
            int jump = emit_jump(chunk, OP_JMP);

            /* Compile the function body and check if it is void. */
            bool is_void = true;
            for (size_t i = 0; i < stmt.as.stmt_fn.stmts.count; i++) {
                if (stmt.as.stmt_fn.stmts.data[i].kind == STMT_RETURN) {
                    is_void = false;
                }
                compile(compiler, chunk, stmt.as.stmt_fn.stmts.data[i], true);
            }

            /* If the function does not have a return statement,
             * emit OP_NULL because we have to return something. */
            if (is_void) {
                emit_bytes(chunk, 2, OP_NULL, OP_RET);
            }

            /* Finally, patch the jump. */
            patch_jump(chunk, jump);

            break;
        }
        case STMT_RETURN: {
            /* Compile the return value and emit OP_RET. */
            compile_expression(compiler, chunk, stmt.as.stmt_return.returnval);
            emit_byte(chunk, OP_RET);
            break;
        }
        default: assert(0);
    }
}