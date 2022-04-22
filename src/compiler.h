#ifndef venom_compiler_h
#define venom_compiler_h

#include <stdint.h>
#include "dynarray.h"
#include "parser.h"
#include "vm.h"

typedef enum {
    OP_PRINT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CONST,
    OP_EXIT,
} Opcode;

typedef DynArray(uint8_t) Uint8DynArray;

typedef struct BytecodeChunk {
    Uint8DynArray code;
} BytecodeChunk;

void init_chunk(BytecodeChunk *chunk);
void free_chunk(BytecodeChunk *chunk);
void compile(BytecodeChunk *chunk, VM *vm, Statement stmt);

#endif