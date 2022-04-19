#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "tokenizer.h"

#define venom_debug

void free_ast(BinaryExpression *binexp) {
    if (binexp->lhs.kind != LITERAL) {
        free_ast(binexp->lhs.data.binexp);
    }

    if (binexp->rhs.kind != LITERAL) {
        free_ast(binexp->rhs.data.binexp);
    }

    free(binexp);
}

static void advance(Parser *parser, Tokenizer *tokenizer) {
    parser->previous = parser->current;
    parser->current = get_token(tokenizer);
}

static bool check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser *parser, Tokenizer *tokenizer, int size, ...) {
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i) {
        TokenType type = va_arg(ap, TokenType);
        if (check(parser, type)) {
            advance(parser, tokenizer);
            va_end(ap);
            return true;
        }
    }
    va_end(ap);
    return false;
}

static void consume(Parser *parser, Tokenizer *tokenizer, TokenType type) {
    if (check(parser, type)) advance(parser, tokenizer);
}

static Expression number(Parser *parser) {
    Expression expr;
    expr.kind = LITERAL,
    expr.data.intval = strtod(parser->previous.start, NULL);
    return expr;
}

static Expression primary();

static ExpressionKind operator(Token token) {
    switch (token.type) {
        case TOKEN_PLUS:  return ADD;
        case TOKEN_MINUS: return SUB;
        case TOKEN_STAR:  return MUL;
        case TOKEN_SLASH: return DIV;
        default:
            assert(0);
     }
}

static Expression factor(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = primary(parser, tokenizer);
    while (match(parser, tokenizer, 2, TOKEN_STAR, TOKEN_SLASH)) {
        ExpressionKind kind = operator(parser->previous);
        Expression right = primary(parser, tokenizer);
        Expression result = { .kind = kind, .data.binexp = malloc(sizeof(BinaryExpression)) };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression term(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = factor(parser, tokenizer);
    while (match(parser, tokenizer, 2, TOKEN_PLUS, TOKEN_MINUS)) {
        ExpressionKind kind = operator(parser->previous);
        Expression right = factor(parser, tokenizer);
        Expression result = { .kind = kind, .data.binexp = malloc(sizeof(BinaryExpression)) };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression expression(Parser *parser, Tokenizer *tokenizer) {
    return term(parser, tokenizer);
}

static Expression grouping(Parser *parser, Tokenizer *tokenizer) {
    Expression exp = expression(parser, tokenizer);
    consume(parser, tokenizer, TOKEN_RIGHT_PAREN);
    return exp;
}

static Expression primary(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_NUMBER)) {
        return number(parser);
    }
    else if (match(parser, tokenizer, 1, TOKEN_LEFT_PAREN)) {
        return grouping(parser, tokenizer);
    }
    else assert(0);
}

#ifdef venom_debug
static void print_expression(const BinaryExpression *e, ExpressionKind kind) {
    printf("(");
    
    if (e->lhs.kind != LITERAL) { 
        print_expression(e->lhs.data.binexp, e->lhs.kind);
    }

    if (e->rhs.kind != LITERAL) {
        print_expression(e->rhs.data.binexp, e->rhs.kind);
    }

    if (e->lhs.kind == LITERAL) {
        printf("%d ", e->lhs.data.intval);
    }

    switch (kind) {
        case ADD: printf("+ "); break;
        case SUB: printf("- "); break;
        case MUL: printf("* "); break;
        case DIV: printf("/ "); break;
        default:
            break;
    }
    
    if (e->rhs.kind == LITERAL) {
        printf("%d", e->rhs.data.intval);
    }

    printf(") ");

    if (!e) printf("\n");
}
#endif

static Statement print_statement(Parser *parser, Tokenizer *tokenizer) {
    Expression exp = expression(parser, tokenizer);
    print_expression(exp.data.binexp, exp.kind);
    Statement stmt = { .kind = STATEMENT_PRINT, .exp = exp };
    consume(parser, tokenizer, TOKEN_SEMICOLON);
    return stmt;
}

static Statement statement(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_PRINT)) {
        return print_statement(parser, tokenizer);
    }
}

Statement parse(Parser *parser, Tokenizer *tokenizer) {
    advance(parser, tokenizer);
    return statement(parser, tokenizer);
}