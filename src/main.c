#include "compiler.h"
#include "tokenizer.h"
#include "parser.h"
#include "vm.h"

int main(int argc, char *argv[]) {
    char *source = "print 6 * 4;";
    Tokenizer tokenizer;
    Parser parser;
    BytecodeChunk chunk;
    VM vm;
    init_tokenizer(&tokenizer, source);
    init_vm(&vm);
    Statement stmt = parse(&parser, &tokenizer);
    compile(&chunk, &vm, stmt);
    run(&vm, &chunk);
    free_vm(&vm);
    free_chunk(&chunk);
    free_ast(stmt.exp.data.binexp);
}