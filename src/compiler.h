#ifndef venom_compiler_h
#define venom_compiler_h

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

typedef struct BytecodeChunk {
    char *code;
    char *ip;
    int count;
} BytecodeChunk;

void free_chunk(BytecodeChunk *chunk);
void compile(BytecodeChunk *chunk, VM *vm, Statement stmt);

#endif