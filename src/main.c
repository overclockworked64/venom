#include "tokenizer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    char *source = "print (1 + 2) * 3;";
    init_tokenizer(source);
    parse();
}