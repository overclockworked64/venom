#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "vm.h"
#include "object.h"

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

#define READ_UINT8() (*++ip)

#define READ_INT16(offset) \
    /* ip points to one of the jump instructions and there \
     * is a 2-byte operand (offset) that comes after the jump \
     * instruction. We want to increment the ip so it points \
     * to the last of the two operands, and construct a 16-bit \
     * offset from the two bytes. Then ip is incremented in \
     * the loop again so it points to the next instruction \
     * (as opposed to pointing somewhere in the middle). */ \
    (ip += 2 + offset, \
    (int16_t)((ip[-1] << 8) | ip[0]))

#define PRINT_STACK() \
do { \
    printf("stack: ["); \
    for (size_t i = 0; i < vm->tos; ++i) { \
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
        printf("current instruction: ");
        switch (*ip) {
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
            case OP_GET_LOCAL: printf("OP_GET_LOCAL"); break;
            case OP_EXIT: printf("OP_EXIT"); break;
        }
        printf("\n");

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
                uint8_t name_index = READ_UINT8();
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
                push(vm, AS_NUM(chunk->cp[READ_UINT8()]));
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
                push(vm, AS_NUM(READ_UINT8()));
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t name_index = READ_UINT8();
                char *name = chunk->sp[name_index];
                for (size_t i = 0; i < vm->locals.count; ++i) {
                    if (strcmp(vm->locals.data[i].name, name) == 0) {
                        push(vm, vm->locals.data[i]);
                        break;
                    }
                }
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
                /* Jump if zero. */
                int16_t offset = READ_INT16(0);
                if (!BOOL_VAL(pop(vm))) {
                    ip += offset;
                }
                break;
            }
            case OP_JMP: {
                int16_t offset = READ_INT16(0);
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
            case OP_FUNC: {
                /* At this point, ip points to OP_FUNC. 
                 * After the opcode, there is the index
                 * of the function's name in the string
                 * constant pool, followed by the number
                 * of function parameters. */
                uint8_t funcname_index = READ_UINT8();
                uint8_t paramcount = READ_UINT8();

                /* After that come the parameters, that is,
                 * their indices in the string constant pool. */
                for (int i = 0; i < paramcount; ++i) {
                    uint8_t name_index = READ_UINT8();
                    dynarray_insert(&vm->locals, (Object){ .name = chunk->sp[name_index] });
                }

                /* After the parameters, there are 3 more bytes.
                 * The first byte is the location of the start of
                 * the function, and the other two bytes comprise 
                 * the size of the function in bytes. */
                uint8_t location = READ_UINT8();
                int16_t size = READ_INT16(1);

                // /* We make the function object... */
                // Function func = {
                //     .size = size,
                //     .location = location,
                //     .name = chunk->sp[funcname_index],
                // };
 
                // Object obj = {
                //     .type = OBJ_FUNCTION,
                //     .as.func = func,
                // };

                printf("location of the func to be invoked is: %d\n", location);
                push(vm, AS_POINTER(&chunk->code.data[location]));

                // /* ...and push it on the stack. */ 
                // push(vm, obj);

                /* Finally, since ip now points to the second byte of the
                 * offset, we modify it so that it points to OP_JMP, because
                 * we don't want to execute the bytecode that comes now. */
                ip -= 3;

                break;
            }
            case OP_INVOKE: {
                /* We first read the argcount. */
                uint8_t argcount = READ_UINT8();

                /* Then, we store the arguments currently
                 * located on the stack in an array. */
                Object arguments[256];
                for (int i = argcount - 1; i >= 0; --i) {
                    arguments[i] = pop(vm);
                }

                /* Then, we update vm->locals with the values
                 * that we are invoking the function with. */
                for (int i = argcount - 1; i >= 0; --i) {
                    vm->locals.data[i].type = arguments[i].type;
                    vm->locals.data[i].as.dval = arguments[i].as.dval;
                }

                /* After we popped the arguments and updated
                 * vm->locals, we expect to see the location
                 * (address) of the function we're invoking. */
                Object obj = pop(vm);


                /* Then, we push the return address on the stack. */
                push(vm, AS_POINTER(ip));
                printf("pushed return addr: %d\n", *ip);

                /* We modify ip so that it points to one instruction
                 * just before the code we're invoking. */
                ip = obj.as.ptr - 1;
                push(vm, obj);
                printf("about to invoke: %d\n", *ip);

                break;
            }
            case OP_RET: {
                /* By the time we encounter OP_RET, the return
                 * value is located on the stack. Beneath it is
                 * the return address. We need to get the return
                 * address in order to modify ip and return to the
                 * called. We need to first pop both of them: */
                Object returnvalue = pop(vm);
                Object returnaddr = pop(vm);

                /* Then, we put the return value back on the stack. */
                push(vm, returnvalue);

                /* Finally, we modify the instruction pointer. */
                ip = returnaddr.as.ptr;

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
#undef READ_INT16
}