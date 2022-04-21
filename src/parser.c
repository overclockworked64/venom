#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "dynarray.h"
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

static void parse_error(Parser *parser, char *message) {
    parser->had_error = true;
    fprintf(stderr, "parse error: %s\n", message);
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

static void consume(Parser *parser, Tokenizer *tokenizer, TokenType type, char *message) {
    if (check(parser, type)) advance(parser, tokenizer);
    else parse_error(parser, message);
}

static Expression number(Parser *parser) {
    Expression expr;
    expr.kind = LITERAL,
    expr.data.val = strtod(parser->previous.start, NULL);
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
    consume(parser, tokenizer, TOKEN_RIGHT_PAREN, "Unmatched closing parentheses.");
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
    } else {
        printf("%f ", e->lhs.data.val);
    }

    switch (kind) {
        case ADD: printf("+ "); break;
        case SUB: printf("- "); break;
        case MUL: printf("* "); break;
        case DIV: printf("/ "); break;
        default:
            break;
    }
    
    if (e->rhs.kind != LITERAL) {
        print_expression(e->rhs.data.binexp, e->rhs.kind);
    } else {
        printf("%f", e->rhs.data.val);
    }

    printf(")");
}
#endif

static Statement print_statement(Parser *parser, Tokenizer *tokenizer) {
    Expression exp = expression(parser, tokenizer);
#ifdef venom_debug
    print_expression(exp.data.binexp, exp.kind);
    printf("\n");
#endif
    Statement stmt = { .kind = STATEMENT_PRINT, .exp = exp };
    consume(parser, tokenizer, TOKEN_SEMICOLON, "Expected semicolon at the end of the expression.");
    return stmt;
}

static Statement statement(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_PRINT)) {
        return print_statement(parser, tokenizer);
    }
}

Statement parse_statement(Parser *parser, Tokenizer *tokenizer) {
    return statement(parser, tokenizer);
}

void parse(Parser *parser, Tokenizer *tokenizer, Statement_DynArray *stmts) {
    parser->had_error = false;
    advance(parser, tokenizer);
    while (parser->current.type != TOKEN_EOF) {
        dynarray_insert(stmts, parse_statement(parser, tokenizer));
    }
}
