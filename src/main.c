#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "tokenizer.h"
#include "parser.h"
#include "vm.h"

void repl() {
    Tokenizer tokenizer;
    Parser parser;
    BytecodeChunk chunk;
    VM vm;

    init_vm(&vm);
    for (;;) {
        size_t len = 0;
        char *line = NULL;

        printf("> ");

        if (getline(&line, &len, stdin) == EOF) {
            free(line);
            break; 
        }

        init_tokenizer(&tokenizer, line);
        Statement stmt = parse(&parser, &tokenizer);
        compile(&chunk, &vm, stmt);
        run(&vm, &chunk);

        free_chunk(&chunk);
        free_ast(stmt.exp.data.binexp);
        free(line);
    }

    free_vm(&vm);
}

int main(int argc, char *argv[]) {
    if (argc == 1) repl();
}