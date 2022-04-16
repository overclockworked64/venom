#include "compiler.h"
#include "tokenizer.h"
#include "parser.h"
#include "vm.h"

int main(int argc, char *argv[]) {
    char *source = "print (1 - 2) * 3;";
    init_tokenizer(source);
    compile(parse());
    run();
}