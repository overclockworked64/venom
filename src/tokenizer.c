#include <stdbool.h>
#include <string.h>
#include "tokenizer.h"

void init_tokenizer(Tokenizer *tokenizer, char *source) {
    tokenizer->current = source;
}

static char peek(Tokenizer *tokenizer) {
    return *tokenizer->current;
}

static char advance(Tokenizer *tokenizer) {
    return *tokenizer->current++;
}

static void skip_whitespace(Tokenizer *tokenizer) {
    while (true) {
        switch (peek(tokenizer)) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
                advance(tokenizer);
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

static bool lookahead(Tokenizer *tokenizer, int length, char *rest) {
    if (strncmp(tokenizer->current, rest, length) == 0) {
        for (int i = 0; i < length; ++i) advance(tokenizer);
        return true;
    }
    return false;
}

static Token make_token(Tokenizer *tokenizer, TokenType type, int length) {
    Token token;
    token.type = type;
    token.start = tokenizer->current - length;
    token.length = length;
    return token;
}

static Token number(Tokenizer *tokenizer) {
    int length = 0;
    while (is_digit(peek(tokenizer))) {
        advance(tokenizer);
        ++length;
    }
    return make_token(tokenizer, TOKEN_NUMBER, length + 1);
}

static Token identifier(Tokenizer *tokenizer) {
    int length = 0;
    while (is_digit(peek(tokenizer)) || is_alpha(peek(tokenizer))) {
        advance(tokenizer);
        ++length;
    }
    return make_token(tokenizer, TOKEN_NUMBER, length + 1);
}

Token get_token(Tokenizer *tokenizer) {
    skip_whitespace(tokenizer);

    char c = advance(tokenizer);
    if (is_digit(c)) return number(tokenizer);
    if (c == '\0') return make_token(tokenizer, TOKEN_EOF, 0);
    switch (c) {
        case '+': return make_token(tokenizer, TOKEN_PLUS, 1);
        case '-': return make_token(tokenizer, TOKEN_MINUS, 1);
        case '*': return make_token(tokenizer, TOKEN_STAR, 1);
        case '/': return make_token(tokenizer, TOKEN_SLASH, 1);
        case '(': return make_token(tokenizer, TOKEN_LEFT_PAREN, 1);
        case ')': return make_token(tokenizer, TOKEN_RIGHT_PAREN, 1);
        case ';': return make_token(tokenizer, TOKEN_SEMICOLON, 1);
        case 'p':
            if (lookahead(tokenizer, 4, "rint")) {
                return make_token(tokenizer, TOKEN_PRINT, 5);
            }
            return identifier(tokenizer);
        default:
            return identifier(tokenizer);
    }
}