#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "tokenizer.h"

#define venom_debug

static void advance() {
    parser.previous = parser.current;
    parser.current = get_token();
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void consume(TokenType type) {
    if (check(type)) advance();
}

static Expression number() {
    Expression expr;
    expr.kind = LITERAL,
    expr.data.intval = strtod(parser.previous.start, NULL);
    return expr;
}

static Expression primary();

static Expression factor() {
    Expression expr = primary();
    while (match(TOKEN_STAR)) {
        Expression right = primary();
        Expression result = { .kind=MUL, .data.binexp=malloc(sizeof(BinaryExpression)) };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression term() {
    Expression expr = factor();
    while (match(TOKEN_PLUS)) {
        Expression right = factor();
        Expression result = { .kind=ADD, .data.binexp=malloc(sizeof(BinaryExpression)) };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression expression() {
    return term();
}

static Expression grouping() {
    Expression exp = expression();
    consume(TOKEN_RIGHT_PAREN);
    return exp;
}

static Expression primary() {
    if (match(TOKEN_NUMBER)) return number();
    else if (match(TOKEN_LEFT_PAREN)) return grouping();
    else exit(1);
}

#ifdef venom_debug
static void print_expression(BinaryExpression *e) {
    printf("(");
    
    if (e->lhs.kind != LITERAL) print_expression(e->lhs.data.binexp);
    if (e->rhs.kind != LITERAL) print_expression(e->rhs.data.binexp);

    if (e->lhs.kind == LITERAL) {
        printf("%d ", e->lhs.data.intval);
    }
    
    if (e->rhs.kind == LITERAL) {
        printf("%d", e->rhs.data.intval);
    }   
   
    printf(") ");

    if (!e) printf("\n");
}
#endif

static void print_statement() {
    Expression e = expression();

    print_expression(e.data.binexp);

    consume(TOKEN_SEMICOLON);
}

static void statement() {
    if (match(TOKEN_PRINT)) print_statement();
}

void parse() {
    advance();
    statement();
}