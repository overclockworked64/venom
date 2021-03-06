#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "math.h"
#include "compiler.h"
#include "vm.h"
#include "object.h"

#define venom_debug

void init_vm(VM *vm) {
    memset(vm, 0, sizeof(VM));
}

void free_vm(VM* vm) {
    /* Free the globals table and its strings. */
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
#define BINARY_OP(op, wrapper) \
do { \
    /* Operands are already on the stack. */ \
    Object b = pop(vm); \
    Object a = pop(vm); \
    push(vm, wrapper(NUM_VAL(a) op NUM_VAL(b))); \
} while (0)

#define READ_UINT8() (*++ip)

#define READ_INT16() \
    /* ip points to one of the jump instructions and there \
     * is a 2-byte operand (offset) that comes after the jump \
     * instruction. We want to increment the ip so it points \
     * to the last of the two operands, and construct a 16-bit \
     * offset from the two bytes. Then ip is incremented in \
     * the loop again so it points to the next instruction \
     * (as opposed to pointing somewhere in the middle). */ \
    (ip += 2, \
    (int16_t)((ip[-1] << 8) | ip[0]))

#define PRINT_STACK() \
do { \
    printf("stack: ["); \
    for (size_t i = 0; i < vm->tos; i++) { \
        print_object(&vm->stack[i]); \
        printf(", "); \
    } \
    printf("]\n"); \
} while(0)

#ifdef venom_debug
    disassemble(chunk);
#endif

    for (
        uint8_t *ip = chunk->code.data;
        ip < &chunk->code.data[chunk->code.count];  /* ip < addr of just beyond the last instruction */
        ip++
    ) {

#ifdef venom_debug
        printf("current instruction: ");
        switch (*ip) {
            case OP_PRINT: printf("OP_PRINT"); break;
            case OP_ADD: printf("OP_ADD"); break;
            case OP_SUB: printf("OP_SUB"); break;
            case OP_MUL: printf("OP_MUL"); break;
            case OP_DIV: printf("OP_DIV"); break;
            case OP_MOD: printf("OP_MOD"); break;
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
            case OP_STR: printf("OP_STR"); break;
            case OP_SET_GLOBAL: printf("OP_SET_GLOBAL"); break;
            case OP_GET_GLOBAL: printf("OP_GET_GLOBAL"); break;
            case OP_DEEP_SET: {
                printf("OP_DEEP_SET: %d", ip[1]);
                break;
            }
            case OP_DEEP_GET: {
                printf("OP_DEEP_GET: %d", ip[1]);
                break;
            }
            case OP_NULL: printf("OP_NULL"); break;
            case OP_EXIT: printf("OP_EXIT"); break;
        }
        printf("\n");
#endif

        switch (*ip) {  /* instruction pointer */
            case OP_PRINT: {
                Object object = pop(vm);
#ifdef venom_debug
                printf("dbg print :: ");
#endif
                print_object(&object);
                printf("\n");
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
                uint8_t name_index = READ_UINT8();
                Object *obj = table_get(&vm->globals, chunk->sp[name_index]);
                if (obj == NULL) {
                    char msg[512];
                    snprintf(
                        msg, sizeof(msg),
                        "Variable '%s' is not defined",
                        chunk->sp[name_index]
                    );
                    runtime_error(msg);
                    return;
                }
                push(vm, *obj);
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
                uint8_t name_index = READ_UINT8();
                Object constant = pop(vm);
                table_insert(&vm->globals, chunk->sp[name_index], constant);
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
                uint8_t index = READ_UINT8();
                push(vm, AS_NUM(chunk->cp[index]));
                break;
            }
            case OP_STR: {
                /* At this point, ip points to OP_CONST.
                 * Since this is a 2-byte instruction with an
                 * immediate operand (the index of the double
                 * constant in the constant pool), we want to
                 * increment the ip to point to the index of
                 * the constant in the constant pool that comes
                 * after the opcode, and push the constant on
                 * the stack. */
                uint8_t index = READ_UINT8();
                push(vm, AS_STR(chunk->sp[index]));
                break;
            }
            case OP_DEEP_SET: {
                uint8_t index = READ_UINT8();
                Object obj = pop(vm);
                int fp = vm->fp_stack[vm->fp_count-1];
                vm->stack[fp+index] = obj;
                break;
            }
            case OP_DEEP_GET: {
                uint8_t index = READ_UINT8();
                int fp = vm->fp_stack[vm->fp_count-1];
                push(vm, vm->stack[fp+index]);
                break;
            }
            case OP_ADD: BINARY_OP(+, AS_NUM); break;
            case OP_SUB: BINARY_OP(-, AS_NUM); break;
            case OP_MUL: BINARY_OP(*, AS_NUM); break;
            case OP_DIV: BINARY_OP(/, AS_NUM); break;
            case OP_MOD: {
                Object b = pop(vm);
                Object a = pop(vm);
                push(vm, AS_NUM(fmod(NUM_VAL(a), NUM_VAL(b))));
                break;
            }
            case OP_GT: BINARY_OP(>, AS_BOOL); break;
            case OP_LT: BINARY_OP(<, AS_BOOL); break;
            case OP_EQ: BINARY_OP(==, AS_BOOL); break;
            case OP_JZ: {
                /* Jump if zero. */
                int16_t offset = READ_INT16();
                if (!BOOL_VAL(pop(vm))) {
                    ip += offset;
                }
                break;
            }
            case OP_JMP: {
                int16_t offset = READ_INT16();
                ip += offset;
                break;
            }
            case OP_NEGATE: {
                Object obj = pop(vm);
                push(vm, AS_NUM(-NUM_VAL(obj)));
                break;
            }
            case OP_NOT: {
                Object obj = pop(vm);
                push(vm, AS_BOOL(BOOL_VAL(obj) ^ 1));
                break;
            }
            case OP_FUNC: {
                /* At this point, ip points to OP_FUNC. 
                 * After the opcode, there is the index
                 * of the function's name in the string
                 * constant pool, followed by the number
                 * of function parameters. */
                uint8_t funcname_index = READ_UINT8();
                uint8_t paramcount = READ_UINT8();

                /* After the number of parameters, there
                 * is one more byte: the location of the
                 * function in the bytecode. */ 
                uint8_t location = READ_UINT8();

                /* We make the function object... */
                char *funcname = chunk->sp[funcname_index];
                Function func = {
                    .location = location,
                    .name = funcname,
                    .paramcount = paramcount,
                };
 
                Object funcobj = {
                    .type = OBJ_FUNCTION,
                    .as.func = func,
                };

                /* ...and insert it into the 'vm->globals' table. */
                table_insert(&vm->globals, funcname, funcobj);

                break;
            }
            case OP_INVOKE: {
                /* We first read the index of the function name and the argcount. */
                uint8_t funcname = READ_UINT8();
                uint8_t argcount = READ_UINT8();

                /* Then, we look it up from the globals table. */ 
                Object *funcobj = table_get(&vm->globals, chunk->sp[funcname]);
                if (funcobj == NULL) {
                    /* Runtime error if the function is not defined. */
                    char msg[512];
                    snprintf(
                        msg, sizeof(msg),
                        "Variable '%s' is not defined",
                        chunk->sp[funcname]
                    );
                    runtime_error(msg);
                    return;
                }

                /* If the number of arguments the function was called with 
                 + does not match the number of parameters the function was
                 * declared to accept, raise a runtime error. */
                if (argcount != funcobj->as.func.paramcount) {
                    char msg[512];
                    snprintf(
                        msg, sizeof(msg),
                        "Function '%s' requires '%d' arguments.",
                        chunk->sp[funcname], argcount
                    );
                    runtime_error(msg);
                    return;
                }

                /* Since we need the arguments after the instruction pointer,
                 * pop them into a temporary array so we can push them back
                 * after we place the instruction pointer on the stack. */
                Object arguments[256];
                for (int i = 0; i < argcount; i++) {
                    arguments[i] = pop(vm);
                }
                
                /* Then, we push the return address on the stack. */
                push(vm, AS_POINTER(ip));

                /* After that, we push the current frame pointer
                 * on the frame pointer stack. */
                vm->fp_stack[vm->fp_count++] = vm->tos;

                /* Push the arguments back on the stack, but in reverse order. */
                for (int i = argcount-1; i >= 0; i--) {
                    push(vm, arguments[i]);
                }
                                
                /* We modify ip so that it points to one instruction
                 * just before the code we're invoking. */
                ip = &chunk->code.data[funcobj->as.func.location-1];

                break;
            }
            case OP_RET: {
                /* By the time we encounter OP_RET, the return
                 * value is located on the stack. Beneath it are
                 * the function arguments, followed by the return
                 * address. */
                Object returnvalue = pop(vm);

                /* We pop the last frame pointer off the frame pointer stack. */
                int fp = vm->fp_stack[--vm->fp_count];

                /* Then, we clean up everything between the top of the stack
                 * and the frame pointer we popped in the previous step. */
                int to_pop = vm->tos - fp;
                for (int i = 0; i < to_pop; i++) {
                    pop(vm);
                }

                /* After the arguments comes the return address which we'll
                 * use to modify the instruction pointer ip and return to the
                 * caller. */
                Object returnaddr = pop(vm);

                /* Then, we push the return value back on the stack.  */
                push(vm, returnvalue);


                /* Finally, we modify the instruction pointer. */
                ip = returnaddr.as.ptr;

                break;
            }
            case OP_TRUE: {
                push(vm, AS_BOOL(true));
                break;
            }
            case OP_NULL: {
                push(vm, (Object){ .type = OBJ_NULL });
                break;
            }
            case OP_EXIT: return;
            default: break;
        }
#ifdef venom_debug
        PRINT_STACK();
#endif
    }
#undef BINARY_OP
#undef READ_UINT8
#undef READ_INT16
}