#ifndef venom_tokenizer_h
#define venom_tokenizer_h

typedef enum {
    TOKEN_PRINT,
    TOKEN_NUMBER,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_STAR,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_EOF,
    TOKEN_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    char *start;
    int length;
} Token;

typedef struct {
    char *current;
} Tokenizer;

Tokenizer tokenizer;

void init_tokenizer(char *source);
Token get_token();

#endif