#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "dynarray.h"
#include "tokenizer.h"
#include "parser.h"
#include "vm.h"

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *buffer = malloc(size+1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read < size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

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

void run_file(char *file) {
    char *source = read_file(file);

    Statement_DynArray stmts = {0};
    Parser parser;
    Tokenizer tokenizer;
    init_tokenizer(&tokenizer, source);
    parse(&parser, &tokenizer, &stmts);

    BytecodeChunk chunk;
    init_chunk(&chunk);
    for (int i = 0; i < stmts.count; i++) {
        compile(&chunk, stmts.data[i]);
        free_stmt(stmts.data[i]);
    }

    VM vm;
    init_vm(&vm);
    run(&vm, &chunk);

    dynarray_free(&stmts);
    free_chunk(&chunk);
    free_vm(&vm);
    free(source);

}

int main(int argc, char *argv[]) {
    if (argc == 1) repl();
    else if (argc == 2) run_file(argv[1]);
}