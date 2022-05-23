#ifndef venom_tokenizer_h
#define venom_tokenizer_h

#define venom_debug

typedef enum {
    TOKEN_PRINT,
    TOKEN_LET,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_BANG,
    TOKEN_GREATER,
    TOKEN_LESS,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_EQUAL,
    TOKEN_DOUBLE_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FN,
    TOKEN_RETURN,
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

void init_tokenizer(Tokenizer *tokenizer, char *source);
Token get_token(Tokenizer *tokenizer);

#ifdef venom_debug
void print_token(Token token);
#endif

#endif