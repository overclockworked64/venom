#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "vm.h"

#define venom_debug

void init_vm(VM *vm) {
    memset(vm, 0, sizeof(VM));
}

void free_vm(VM* vm) {
    /* free the globals table and its strings */
    table_free(&vm->globals); 
}

static void runtime_error(const char *message) {
    fprintf(stderr, "runtime error: %s.\n", message);
}

static void push(VM *vm, Object obj) {
    vm->stack[vm->tos++] = obj;
}

static Object pop(VM *vm) {
    return vm->stack[--vm->tos];
}

void run(VM *vm, BytecodeChunk *chunk) {
#define BINARY_OP(vm, op, wrapper) \
do { \
    /* operands are already on the stack */ \
    Object b = pop(vm); \
    Object a = pop(vm); \
    push(vm, wrapper(NUM_VAL(a) op NUM_VAL(b))); \
} while (0)

#define READ16() \
    /* ip points to one of the jump instructions and there \
     * is a 2-byte operand (offset) that comes after the jump \
     * instruction. We want to increment the ip so it points \
     * to the last of the two operands, and construct a 16-bit \
     * offset from the two bytes. Then ip is incremented in \
     * the loop again so it points to the next instruction \
     * (as opposed to pointing somewhere in the middle). */ \
    (ip += 2, \
    (uint16_t)((ip[-1] << 8) | ip[0]))

#ifdef venom_debug
    disassemble(chunk);
#endif

    for (
        uint8_t *ip = chunk->code.data;
        ip < &chunk->code.data[chunk->code.count];  /* ip < addr of just beyond the last instruction */
        ip++
    ) {
        switch (*ip) {  /* instruction pointer */
            case OP_PRINT: {
                Object object = pop(vm);
#ifdef venom_debug
                printf("dbg print :: ");
#endif
                print_object(&object);
                break;
            }
            case OP_GET_GLOBAL: {
                /* At this point, ip points to OP_GET_GLOBAL.
                 * Since this is a 2-byte instruction with an
                 * immediate operand (the index of the name of
                 * the looked up variable in the string constant
                 * pool), we want to increment the ip so it points
                 * to the /index/ of the string in the string
                 * constant pool that comes after the opcode.
                 * We then look up the variable and push its value
                 * on the stack. If we can't find the variable,
                 * we bail out. */
                uint8_t name_index = *++ip;
                Object *value = table_get(&vm->globals, chunk->sp[name_index]);
                if (value == NULL) {
                    char msg[512];
                    snprintf(msg, sizeof(msg), "Variable '%s' is not defined", chunk->sp[name_index]);
                    runtime_error(msg);
                    return;
                }
                push(vm, *value);
                break;
            }
            case OP_SET_GLOBAL: {
                /* At this point, ip points to OP_SET_GLOBAL.
                 * This is a single-byte instruction that expects
                 * two things to already be on the stack: the index
                 * of the variable name in the string constant pool,
                 * and the value of the double constant that the name
                 * refers to. We pop these two and add the variable
                 * to the globals table. */
                Object constant = pop(vm);
                Object name_index = pop(vm);
                table_insert(&vm->globals, chunk->sp[(int)NUM_VAL(name_index)], constant);
                break;
            }
            case OP_CONST: {
                /* At this point, ip points to OP_CONST.
                 * Since this is a 2-byte instruction with an
                 * immediate operand (the index of the double
                 * constant in the constant pool), we want to
                 * increment the ip to point to the index of
                 * the constant in the constant pool that comes
                 * after the opcode, and push the constant on
                 * the stack. */
                push(vm, AS_NUM(chunk->cp[*++ip]));
                break;
            }
            case OP_STR_CONST: {
                /* At this point, ip points to OP_STR_CONST.
                 * Since this is a 2-byte instruction with an
                 * immediate operand (the index of the string
                 * constant in the string constant pool), we
                 * want to increment the ip so it points to
                 * what comes after the opcode, and push the
                 * /index/ of the string constant on the stack. */
                push(vm, AS_NUM(*++ip));
                break;
            }
            case OP_ADD: BINARY_OP(vm, +, AS_NUM); break;
            case OP_SUB: BINARY_OP(vm, -, AS_NUM); break;
            case OP_MUL: BINARY_OP(vm, *, AS_NUM); break;
            case OP_DIV: BINARY_OP(vm, /, AS_NUM); break;
            case OP_GT: BINARY_OP(vm, >, AS_BOOL); break;
            case OP_LT: BINARY_OP(vm, <, AS_BOOL); break;
            case OP_EQ: BINARY_OP(vm, ==, AS_BOOL); break;
            case OP_JZ: {
                uint16_t offset = READ16();
                if (!BOOL_VAL(pop(vm))) {
                    /* Jump over the 'then' branch. After we jump over
                     * the 'then' branch, we land on OP_JMP, which will
                     * have '0' as offset if there is no 'else' branch
                     * coming. Otherwise, it means we didn't take the 
                     * 'then' branch and we will take the upcoming branch. */
                    ip += offset;
                }
                break;
            }
            case OP_JMP: {
                uint16_t offset = READ16();
                ip += offset;
                break;
            }
            case OP_NEGATE: { 
                push(vm, AS_NUM(-NUM_VAL(pop(vm))));
                break;
            }
            case OP_NOT: {
                push(vm, AS_BOOL(BOOL_VAL(pop(vm)) ^ 1));
                break;
            }
            case OP_EXIT: return;
            default: break;
        }
    }
#undef BINARY_OP
#undef READ16
}