#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "tokenizer.h"

#define venom_debug

void init_tokenizer(Tokenizer *tokenizer, char *source) {
    tokenizer->current = source;
}

static char peek(Tokenizer *tokenizer, int distance) {
    return tokenizer->current[distance];
}

static char advance(Tokenizer *tokenizer) {
    return *tokenizer->current++;
}

static void skip_whitespace(Tokenizer *tokenizer) {
    while (true) {
        switch (peek(tokenizer, 0)) {
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
    return c >= '0' && c <= '9';
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
    while (is_digit(peek(tokenizer, 0))) {
        advance(tokenizer);
        ++length;
    }
    if (peek(tokenizer, 0) == '.' && is_digit(peek(tokenizer, 1))) {
        advance(tokenizer);
        ++length;
 
        while (is_digit(peek(tokenizer, 0))) {
            advance(tokenizer);
            ++length;
        }
    }
    return make_token(tokenizer, TOKEN_NUMBER, length + 1);
}

static Token identifier(Tokenizer *tokenizer) {
    int length = 0;
    while (is_digit(peek(tokenizer, 0)) || is_alpha(peek(tokenizer, 0))) {
        advance(tokenizer);
        ++length;
    }
    return make_token(tokenizer, TOKEN_IDENTIFIER, length + 1);
}

#ifdef venom_debug
void print_token(Token token) {
    printf("current token: Token { .type: ");

    switch (token.type) {
        case TOKEN_PRINT:         printf("TOKEN_PRINT"); break;
        case TOKEN_LET:           printf("TOKEN_LET"); break;
        case TOKEN_IDENTIFIER:    printf("TOKEN_IDENTIFIER"); break;
        case TOKEN_NUMBER:        printf("TOKEN_NUMBER"); break;
        case TOKEN_LEFT_PAREN:    printf("TOKEN_LEFT_PAREN"); break;
        case TOKEN_RIGHT_PAREN:   printf("TOKEN_RIGHT_PAREN"); break;
        case TOKEN_STAR:          printf("TOKEN_STAR"); break;
        case TOKEN_SLASH:         printf("TOKEN_SLASH"); break;
        case TOKEN_PLUS:          printf("TOKEN_PLUS"); break;
        case TOKEN_MINUS:         printf("TOKEN_MINUS"); break;
        case TOKEN_DOT:           printf("TOKEN_DOT"); break;
        case TOKEN_SEMICOLON:     printf("TOKEN_SEMICOLON"); break;
        case TOKEN_EQUALS:        printf("TOKEN_EQUALS"); break;
        case TOKEN_DOUBLE_EQUALS: printf("TOKEN_DOUBLE_EQUALS"); break;
        case TOKEN_EOF:           printf("TOKEN_EOF"); break;
        case TOKEN_ERROR:         printf("TOKEN_ERROR"); break;
        default: break;
    }

    printf(", value: '%.*s' }\n", token.length, token.start);
}
#endif

Token get_token(Tokenizer *tokenizer) {
    skip_whitespace(tokenizer);

    char c = advance(tokenizer);
    if (c == '\0') return make_token(tokenizer, TOKEN_EOF, 0);
    if (is_digit(c)) return number(tokenizer);
    switch (c) {
        case '+': return make_token(tokenizer, TOKEN_PLUS, 1);
        case '-': return make_token(tokenizer, TOKEN_MINUS, 1);
        case '*': return make_token(tokenizer, TOKEN_STAR, 1);
        case '/': return make_token(tokenizer, TOKEN_SLASH, 1);
        case '(': return make_token(tokenizer, TOKEN_LEFT_PAREN, 1);
        case ')': return make_token(tokenizer, TOKEN_RIGHT_PAREN, 1);
        case ';': return make_token(tokenizer, TOKEN_SEMICOLON, 1);
        case '>': {
            if (lookahead(tokenizer, 1, "=")) {
                return make_token(tokenizer, TOKEN_GREATER_EQUAL, 2);
            }
            return make_token(tokenizer, TOKEN_GREATER, 1);
        }
        case '<': {
            if (lookahead(tokenizer, 1, "=")) {
                return make_token(tokenizer, TOKEN_LESS_EQUAL, 2);
            }
            return make_token(tokenizer, TOKEN_LESS, 1);
        }
        case '!': {
            if (lookahead(tokenizer, 1, "=")) {
                return make_token(tokenizer, TOKEN_BANG_EQUALS, 2);
            }
            return make_token(tokenizer, TOKEN_BANG, 1);
        }
        case '=': {
            if (lookahead(tokenizer, 1, "=")) {
                return make_token(tokenizer, TOKEN_DOUBLE_EQUALS, 2);
            }
            return make_token(tokenizer, TOKEN_EQUALS, 1);
        }
        case 'l': {
            if (lookahead(tokenizer, 2, "et")) {
                return make_token(tokenizer, TOKEN_LET, 3);
            }
            return identifier(tokenizer);
        }
        case 'p': {
            if (lookahead(tokenizer, 4, "rint")) {
                return make_token(tokenizer, TOKEN_PRINT, 5);
            }
            return identifier(tokenizer);
        }
        default:
            return identifier(tokenizer);
    }
}