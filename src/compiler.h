#ifndef venom_compiler_h
#define venom_compiler_h

#include "parser.h"

typedef enum {
    OP_PRINT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CONST,
    OP_EXIT,
} Opcode;

void compile(Statement stmt);

typedef struct {
    char *code;
    char *ip;
    int count;
} BytecodeChunk;

BytecodeChunk compiling_chunk;

void init_chunk(BytecodeChunk *chunk);

#endif