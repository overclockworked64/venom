#include "compiler.h"
#include "tokenizer.h"
#include "parser.h"
#include "vm.h"

int main(int argc, char *argv[]) {
    char *source = "print 256 * 4;";
    init_tokenizer(source);
    init_vm();
    compile(parse());
    run();
}