#include <stdbool.h>
#include <string.h>
#include "tokenizer.h"

typedef struct {
    char *current;
} Tokenizer;

static Tokenizer tokenizer;

void init_tokenizer(char *source) {
    tokenizer.current = source;
}

static char peek() {
    return *tokenizer.current;
}

static char advance() {
    return *tokenizer.current++;
}

static void skip_whitespace() {
    while (true) {
        switch (peek()) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
                advance();
                break;
            default:
                return;
        }
    }
}

static bool is_digit(char c) {
    return c >= '1' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool lookahead(int length, char *rest) {
    if (strncmp(tokenizer.current, rest, length) == 0) {
        for (int i = 0; i < length; ++i) advance();
        return true;
    }
    return false;
}

static Token make_token(TokenType type, int length) {
    Token token;
    token.type = type;
    token.start = tokenizer.current - length;
    token.length = length;
    return token;
}

static Token number() {
    int length = 0;
    while (is_digit(peek())) {
        advance();
        ++length;
    }
    return make_token(TOKEN_NUMBER, length + 1);
}

static Token identifier() {
    int length = 0;
    while (is_digit(peek()) || is_alpha(peek())) {
        advance();
        ++length;
    }
    return make_token(TOKEN_NUMBER, length + 1);
}

Token get_token() {
    skip_whitespace();

    char c = advance();
    if (is_digit(c)) return number();
    if (c == '\0') return make_token(TOKEN_EOF, 0);
    switch (c) {
        case '+': return make_token(TOKEN_PLUS, 1);
        case '-': return make_token(TOKEN_MINUS, 1);
        case '*': return make_token(TOKEN_STAR, 1);
        case '/': return make_token(TOKEN_SLASH, 1);
        case '(': return make_token(TOKEN_LEFT_PAREN, 1);
        case ')': return make_token(TOKEN_RIGHT_PAREN, 1);
        case ';': return make_token(TOKEN_SEMICOLON, 1);
        case 'p':
            if (lookahead(4, "rint")) {
                return make_token(TOKEN_PRINT, 5);
            }
            return identifier();
        default:
            return identifier();
    }
}