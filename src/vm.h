#ifndef venom_vm_h
#define venom_vm_h

#define STACK_MAX 256
#define LOCAL_MAX 256

#include "compiler.h"
#include "dynarray.h"
#include "object.h"
#include "table.h"

typedef struct {
    Object stack[STACK_MAX];
    size_t tos; /* top of stack */
    Table globals;
    Object_DynArray locals;
} VM;

typedef struct BytecodeChunk BytecodeChunk;

void init_vm(VM *vm);
void free_vm(VM *vm);
void run(VM *vm, BytecodeChunk *chunk);

#endif