#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "tokenizer.h"

#define venom_debug

typedef struct {
    Token current;
    Token previous;
} Parser;

static Parser parser;

void free_ast(BinaryExpression *binexp) {
    if (binexp->lhs.kind != LITERAL) free_ast(binexp->lhs.data.binexp);
    if (binexp->rhs.kind != LITERAL) free_ast(binexp->rhs.data.binexp);
    free(binexp);
}

static void advance() {
    parser.previous = parser.current;
    parser.current = get_token();
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(int size, TokenType types[]) {
    for (int i = 0; i < size; ++i) {
        if (check(types[i])) {
            advance();
            return true;
        }
    }
    return false;
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

static ExpressionKind operator(Token token) {
    switch (token.type) {
        case TOKEN_PLUS:  return ADD;
        case TOKEN_MINUS: return SUB;
        case TOKEN_STAR:  return MUL;
        case TOKEN_SLASH: return DIV;
        default:
            break;
     }
}

static Expression factor() {
    Expression expr = primary();
    while (match(2, (TokenType[]) { TOKEN_STAR, TOKEN_SLASH } )) {
        ExpressionKind kind = operator(parser.previous);
        Expression right = primary();
        Expression result = { .kind=kind, .data.binexp=malloc(sizeof(BinaryExpression)) };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression term() {
    Expression expr = factor();
    while (match(2, (TokenType[]) { TOKEN_PLUS, TOKEN_MINUS } )) {
        ExpressionKind kind = operator(parser.previous);
        Expression right = factor();
        Expression result = { .kind=kind, .data.binexp=malloc(sizeof(BinaryExpression)) };
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
    if (match(1, (TokenType[]) { TOKEN_NUMBER } )) return number();
    else if (match(1, (TokenType[]) { TOKEN_LEFT_PAREN } )) return grouping();
    else exit(1);
}

#ifdef venom_debug
static void print_expression(BinaryExpression *e, ExpressionKind kind) {
    printf("(");
    
    if (e->lhs.kind != LITERAL) print_expression(e->lhs.data.binexp, e->lhs.kind);
    if (e->rhs.kind != LITERAL) print_expression(e->rhs.data.binexp, e->rhs.kind);

    if (e->lhs.kind == LITERAL) {
        printf("%d ", e->lhs.data.intval);
    }

    switch (kind) {
        case ADD:
            printf("+ ");
            break;
        case SUB:
            printf("- ");
            break;
        case MUL:
            printf("* ");
            break;
        case DIV:
            printf("/ ");
            break;
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

static Statement print_statement() {
    Expression exp = expression();
    print_expression(exp.data.binexp, exp.kind);
    Statement stmt = { .kind = STATEMENT_PRINT, .exp = exp };
    consume(TOKEN_SEMICOLON);
    return stmt;
}

static Statement statement() {
    if (match(1, (TokenType[]) { TOKEN_PRINT })) return print_statement();
}

Statement parse() {
    advance();
    return statement();
}