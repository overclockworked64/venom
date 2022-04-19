#ifndef venom_vm_h
#define venom_vm_h

#define STACK_MAX 255

#include "compiler.h"

typedef struct {
    int stack[STACK_MAX];
    int tos;  /* top of stack */
    int *cp;  /* constant pool */
    int cpp;  /* constant pool pointer */
} VM;

typedef struct BytecodeChunk BytecodeChunk;

void init_vm(VM *vm);
void free_vm(VM *vm);
void run(VM *vm, BytecodeChunk *chunk);

#endif