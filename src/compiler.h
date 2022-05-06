#ifndef venom_compiler_h
#define venom_compiler_h

#define POOL_MAX 256

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
    OP_NEGATE,
    OP_CONST,
    OP_STR_CONST,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_ASSIGN,
    OP_EXIT,
} Opcode;

typedef DynArray(uint8_t) Uint8DynArray;

typedef struct BytecodeChunk {
    Uint8DynArray code;
    double cp[POOL_MAX];  /* constant pool */
    char *sp[POOL_MAX];   /* string pool */
    uint8_t cp_count;
    uint8_t sp_count;
} BytecodeChunk;

void init_chunk(BytecodeChunk *chunk);
void free_chunk(BytecodeChunk *chunk);
void compile(BytecodeChunk *chunk, Statement stmt);

#endif