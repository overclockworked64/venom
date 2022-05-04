#ifndef venom_vm_h
#define venom_vm_h

#define STACK_MAX 256

#include "compiler.h"
#include "dynarray.h"
#include "table.h"

typedef struct {
    double stack[STACK_MAX];
    size_t tos;           /* top of stack */
    Table globals;
} VM;

typedef struct BytecodeChunk BytecodeChunk;

void init_vm(VM *vm);
void free_vm(VM *vm);
void run(VM *vm, BytecodeChunk *chunk);

#endif