#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "dynarray.h"
#include "tokenizer.h"
#include "parser.h"
#include "vm.h"

void repl() {
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

        Tokenizer tokenizer;
        init_tokenizer(&tokenizer, line);

        Statement_DynArray stmts = {0};
        Parser parser;
        parse(&parser, &tokenizer, &stmts);

        if (parser.had_error) continue;
        
        BytecodeChunk chunk;
        init_chunk(&chunk);
        for (int i = 0; i < stmts.count; i++) {
            compile(&chunk, stmts.data[i]);
            free_stmt(stmts.data[i]);
        }
        run(&vm, &chunk);

        dynarray_free(&stmts);
        free_chunk(&chunk);
        free(line);
    }
    free_vm(&vm);
}

int main(int argc, char *argv[]) {
    if (argc == 1) repl();
}