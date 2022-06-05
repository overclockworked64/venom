#ifndef venom_vm_h
#define venom_vm_h

#define STACK_MAX 256

#include "compiler.h"
#include "dynarray.h"
#include "object.h"
#include "table.h"

typedef struct {
    Object stack[STACK_MAX];
    size_t tos; /* top of stack */
    Table globals;
    int fp_stack[256]; /* a stack for frame pointers */
    size_t fp_count;
} VM;

typedef struct BytecodeChunk BytecodeChunk;

void init_vm(VM *vm);
void free_vm(VM *vm);
void run(VM *vm, BytecodeChunk *chunk);

#endif