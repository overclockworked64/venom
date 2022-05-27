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
            uint8_t const_index = add_constant(chunk, exp.data.dval);
            emit_bytes(chunk, 2, OP_CONST, const_index);
            break;
        }
        case EXP_VARIABLE: {
            uint8_t name_index = add_string(chunk, exp.name);
            if (!scoped) {
                emit_bytes(chunk, 2, OP_GET_GLOBAL, name_index);
            } else {
                int index = var_index(compiler->locals, exp.name);
                if (index == -1) {
                    dynarray_insert(&compiler->locals, exp.name);
                    printf("emitting OP_GET_LOCAL for name: %s with index %ld\n", exp.name, compiler->locals.count - 1);
                    emit_bytes(chunk, 2, OP_GET_LOCAL, compiler->locals.count - 1);
                    printf("emitting oP_GET_LOCAL\n");
                } else {
                    emit_bytes(chunk, 2, OP_GET_LOCAL, index);
                    printf("emitting oP_GET_LOCAL\n");
                }
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
            for (size_t i = 0; i < exp.arguments.count; ++i) {
                compile_expression(compiler, chunk, exp.arguments.data[i], scoped);
            }
            uint8_t funcname_index = add_string(chunk, exp.name);
            emit_bytes(chunk, 2, OP_INVOKE, funcname_index);
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
                i += 2 + paramcount;
                break;
            }
            case OP_INVOKE: {
                uint8_t funcname_index = *++ip;
                char *funcname = chunk->sp[funcname_index];
                printf("%d: ", i);
                printf("OP_INVOKE, byte (funcname_index: '%d' ('%s')\n", funcname_index, funcname);
                ++i;
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
            case OP_DEEP_SET: {
                uint8_t index = *++ip;
                printf("%d: ", i);
                printf("OP_DEEP_SET, byte (%d)\n", index);
                ++i;
                break;
            }
            default: printf("Unknown instruction: %d.\n", *ip); break;
        }
    }
}
#endif

#ifdef venom_debug
void print_instruction(char *prefix, Opcode opcode) {
    printf("%s: ", prefix);
    switch (opcode) {
        case OP_PRINT: printf("OP_PRINT"); break;
        case OP_ADD: printf("OP_ADD"); break;
        case OP_SUB: printf("OP_SUB"); break;
        case OP_MUL: printf("OP_MUL"); break;
        case OP_DIV: printf("OP_DIV"); break;
        case OP_EQ: printf("OP_EQ"); break;
        case OP_GT: printf("OP_GT"); break;
        case OP_LT: printf("OP_LT"); break;
        case OP_NOT: printf("OP_NOT"); break;
        case OP_NEGATE: printf("OP_NEGATE"); break;
        case OP_JMP: printf("OP_JMP"); break;
        case OP_JZ: printf("OP_JZ"); break;
        case OP_FUNC: printf("OP_FUNC"); break;
        case OP_INVOKE: printf("OP_INVOKE"); break;
        case OP_RET: printf("OP_RET"); break;
        case OP_CONST: printf("OP_CONST"); break;
        case OP_STR_CONST: printf("OP_STR_CONST"); break;
        case OP_SET_GLOBAL: printf("OP_SET_GLOBAL"); break;
        case OP_GET_GLOBAL: printf("OP_GET_GLOBAL"); break;
        case OP_SET_LOCAL: printf("OP_SET_LOCAL"); break;
        case OP_GET_LOCAL: printf("OP_GET_LOCAL");break;
        case OP_POP: printf("OP_POP"); break;
        case OP_EXIT: printf("OP_EXIT"); break;
        case OP_DEEP_SET: printf("OP_DEEP_SET"); break;
    }
}
#endif

void backpatch_stack_size(Compiler *compiler, BytecodeChunk *chunk, int index, int size, char *prefix) {
    for (;;) {
        print_instruction(prefix, chunk->code.data[index]);
        switch (chunk->code.data[index]) {
            case OP_CONST:
            case OP_STR_CONST:
            case OP_GET_GLOBAL: {
                size++;
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                index += 2;
                break;
            }
            case OP_GET_LOCAL: {
                if (compiler->stack_sizes.data[index] == 255) {
                    int current_index = chunk->code.data[index+1];
                    printf("size is: %d \t current_index is: %d\n", size, current_index);
                    chunk->code.data[index+1] = size - current_index - 1;
                    compiler->stack_sizes.data[index] = ++size;
                    printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                    printf("OP_GET_LOCAL index after patching is: %d\n", chunk->code.data[index+1]);
                    index += 2;
                    break;
                } else {
                    return;
                }
            }
            case OP_SET_LOCAL: {
                size--;
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                int current_index = chunk->code.data[index+1];
                chunk->code.data[index+1] = size - current_index - 1;
                index += 2;
                break;
            }
            case OP_DEEP_SET: {
                size--;
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                // chunk->code.data[index+1] = size;
                index += 2;
                break;
            }
            case OP_SET_GLOBAL: {
                size -= 2;
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                index++;
                break;
            }
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_EQ:
            case OP_GT:
            case OP_LT:
            case OP_POP:
            case OP_PRINT: {
                size--;
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                index++;
                break;
            }
            case OP_JZ: {
                if (compiler->stack_sizes.data[index] == 255) {
                    int offset = chunk->code.data[index+1];
                    offset <<= 8;
                    offset |= chunk->code.data[index+2];
                    compiler->stack_sizes.data[index] = size - 1;
                    printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                    backpatch_stack_size(compiler, chunk, index+offset+3, size-1, "else");
                    size--;
                    compiler->stack_sizes.data[index] = size;
                    printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                    index += 3;
                    break;
                } else {
                    return;
                }
            }
            case OP_JMP: {
                int offset = chunk->code.data[index+1];
                offset <<= 8;
                offset |= chunk->code.data[index+2];
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                index += offset + 3;
                break;
            }
            case OP_FUNC: {
                uint8_t paramcount = chunk->code.data[index+2];
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                index += 4 + paramcount;                
                break;
            }
            case OP_RET: {
                size--;
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                return;
            }
            case OP_NOT:
            case OP_NEGATE: {
                compiler->stack_sizes.data[index] = size;
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                index++;
                break;
            }
            case OP_INVOKE: {
                if (compiler->stack_sizes.data[index] == 255) {
                    char *funcname = chunk->sp[chunk->code.data[index+1]];
                    Object *location = table_get(&compiler->functions, funcname);
                    size++;
                    compiler->stack_sizes.data[index] = size;
                    printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                    compiler->stack[compiler->tos++] = index+1;
                    index = location->as.dval;
                    break;
                } else {
                    return;
                }
            }
            case OP_EXIT: {
                printf(" stack size: %d\n", compiler->stack_sizes.data[index]);
                return;
            }
            default: printf("Unknown instruction.\n"); break;
        }
    }
}

void compile(Compiler *compiler, BytecodeChunk *chunk, Statement stmt, bool scoped) {
    switch (stmt.kind) {
        case STMT_PRINT: {
            compile_expression(compiler, chunk, stmt.exp, scoped);
            emit_byte(chunk, OP_PRINT);        
            break;
        }
        case STMT_LET:
        case STMT_ASSIGN: {
            compile_expression(compiler, chunk, stmt.exp, scoped);
            if (!scoped) {
                emit_byte(chunk, OP_SET_GLOBAL);
            } else {
                int index = var_index(compiler->locals, stmt.name);
                if (index == -1) {
                    dynarray_insert(&compiler->locals, stmt.name);
                    emit_bytes(chunk, 2, OP_SET_LOCAL, compiler->locals.count - 1);
                } else {
                    emit_bytes(chunk, 2, OP_SET_LOCAL, index);
                }
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
                dynarray_insert(&compiler->locals, stmt.parameters.data[i]);
                uint8_t parameter_index = add_string(chunk, stmt.parameters.data[i]);
                emit_byte(chunk, parameter_index);
            }
           
            /* Emit the location of the start of the function. */
            emit_byte(chunk, (uint8_t)chunk->code.count + 4);

            table_insert(&compiler->functions, stmt.name, (Object){ .type = OBJ_NUMBER, .as.dval = chunk->code.count + 3});
            
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
            for (size_t i = 0; i < compiler->paramcount; ++i) {
                emit_bytes(chunk, 2, OP_DEEP_SET, 1);
            }
            emit_byte(chunk, OP_RET);
            break;
        }
        default: assert(0);
    }
}