#include "compiler.h"
#include "tokenizer.h"
#include "parser.h"
#include "vm.h"

int main(int argc, char *argv[]) {
    char *source = "print 6 * 4;";
    init_tokenizer(source);
    init_vm();
    Statement stmt = parse();
    compile(stmt);
    run();
    free_vm();
    free_chunk();
    free_ast(stmt.exp.data.binexp);
}