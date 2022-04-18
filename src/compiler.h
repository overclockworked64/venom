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

typedef struct {
    char *code;
    char *ip;
    int count;
} BytecodeChunk;

BytecodeChunk chunk;

void init_chunk();
void free_chunk();
void compile(Statement stmt);

#endif