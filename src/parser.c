#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynarray.h"
#include "parser.h"
#include "tokenizer.h"
#include "util.h"

#define venom_debug

void free_stmt(Statement stmt) {
    if (stmt.kind == STMT_LET || stmt.kind == STMT_ASSIGN) free(stmt.name);
    if (stmt.kind == STMT_BLOCK) {
        for (int i = 0; i < stmt.stmts.count; ++i) {
            free_stmt(stmt.stmts.data[i]);
        }
        dynarray_free(&stmt.stmts);
    }
    free_expression(stmt.exp);
}

void free_expression(Expression e) {
    if (e.kind == EXP_LITERAL) return;
    else if (e.kind == EXP_VARIABLE) free(e.name);
    else if (e.kind == EXP_UNARY) {
        free_expression(*e.data.exp);
        free(e.data.exp);
    }
    else {
        free_expression(e.data.binexp->lhs);
        free_expression(e.data.binexp->rhs);
        free(e.data.binexp);
    }
}

static void parse_error(Parser *parser, char *message) {
    parser->had_error = true;
    fprintf(stderr, "parse error: %s\n", message);
}

static Token advance(Parser *parser, Tokenizer *tokenizer) {
    parser->previous = parser->current;
    parser->current = get_token(tokenizer);
#ifdef venom_debug
    print_token(parser->current);
#endif
    return parser->previous;
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

static Token consume(Parser *parser, Tokenizer *tokenizer, TokenType type, char *message) {
    if (check(parser, type)) return advance(parser, tokenizer);
    else parse_error(parser, message);
}

static Expression number(Parser *parser) {
    Expression expr;
    expr.kind = EXP_LITERAL;
    expr.data.dval = strtod(parser->previous.start, NULL);
    return expr;
}

static Expression variable(Parser *parser) {
    char *name = own_string_n(parser->previous.start, parser->previous.length);
    Expression expr = { .kind = EXP_VARIABLE, .name = name };
    return expr;
}

static Expression primary();

static char *operator(Token token) {
    switch (token.type) {
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_DOUBLE_EQUAL: return "==";
        case TOKEN_BANG_EQUAL: return "!=";
        case TOKEN_GREATER: return ">";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_LESS: return "<";
        case TOKEN_LESS_EQUAL: return "<=";
        default:
            assert(0);
     }
}

static Expression unary(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_MINUS)) {
        Expression *right = malloc(sizeof(Expression));
        *right = unary(parser, tokenizer);
        Expression result = { .kind = EXP_UNARY, .data.exp = right };
        return result;
    }
    return primary(parser, tokenizer);
}

static Expression factor(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = unary(parser, tokenizer);
    while (match(parser, tokenizer, 2, TOKEN_STAR, TOKEN_SLASH)) {
        char *op = operator(parser->previous);
        Expression right = unary(parser, tokenizer);
        Expression result = { 
            .kind = EXP_BINARY, 
            .data.binexp = malloc(sizeof(BinaryExpression)), 
            .operator = op, 
        };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression term(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = factor(parser, tokenizer);
    while (match(parser, tokenizer, 2, TOKEN_PLUS, TOKEN_MINUS)) {
        char *op = operator(parser->previous);
        Expression right = factor(parser, tokenizer);
        Expression result = { 
            .kind = EXP_BINARY,
            .data.binexp = malloc(sizeof(BinaryExpression)),
            .operator = op,
        };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression comparison(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = term(parser, tokenizer);
    while (match(parser, tokenizer, 4,
        TOKEN_GREATER, TOKEN_LESS,
        TOKEN_GREATER_EQUAL, TOKEN_LESS_EQUAL
    )) {
        char *op = operator(parser->previous);
        Expression right = term(parser, tokenizer);
        Expression result = { 
            .kind = EXP_BINARY,
            .data.binexp = malloc(sizeof(BinaryExpression)),
            .operator = op,
        };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression equality(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = comparison(parser, tokenizer);
    while (match(parser, tokenizer, 2, TOKEN_DOUBLE_EQUAL, TOKEN_BANG_EQUAL)) {
        char *op = operator(parser->previous);
        Expression right = comparison(parser, tokenizer);
        Expression result = { 
            .kind = EXP_BINARY,
            .data.binexp = malloc(sizeof(BinaryExpression)),
            .operator = op,
        };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression expression(Parser *parser, Tokenizer *tokenizer) {
    return equality(parser, tokenizer);
}

static Expression grouping(Parser *parser, Tokenizer *tokenizer) {
    Expression exp = expression(parser, tokenizer);
    consume(parser, tokenizer, TOKEN_RIGHT_PAREN, "Unmatched closing parentheses.");
    return exp;
}

static Statement statement(Parser *parser, Tokenizer *tokenizer);

static Statement_DynArray block(Parser *parser, Tokenizer *tokenizer) {
    Statement_DynArray stmts = {0};
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        dynarray_insert(&stmts, statement(parser, tokenizer));
    }
    consume(parser, tokenizer, TOKEN_RIGHT_BRACE, "Expected '}' at the end of the block.");
    return stmts;
}

static Expression primary(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_NUMBER)) return number(parser);
    else if (match(parser, tokenizer, 1, TOKEN_IDENTIFIER)) return variable(parser);
    else if (match(parser, tokenizer, 1, TOKEN_LEFT_PAREN)) return grouping(parser, tokenizer);
}

#ifdef venom_debug
static void print_expression(Expression e) {
    printf("(");
    switch (e.kind) {
        case EXP_LITERAL: {
            printf("%f", e.data.dval);
            break;
        }
        case EXP_VARIABLE: {
            printf("%s", e.name);
            break;
        }
        case EXP_UNARY: {
            printf("-");
            print_expression(*e.data.exp);
            break;
        }
        case EXP_BINARY: {
            print_expression(e.data.binexp->lhs);
            printf(" %s ", e.operator);
            print_expression(e.data.binexp->rhs);
            break;
        }
        default: assert(0);
    }
    printf(")");
}
#endif

static Statement print_statement(Parser *parser, Tokenizer *tokenizer) {
    Expression exp = expression(parser, tokenizer);

#ifdef venom_debug
    print_expression(exp);
    printf("\n");
#endif

    Statement stmt = { .kind = STMT_PRINT, .exp = exp, .name = NULL };
    consume(parser, tokenizer, TOKEN_SEMICOLON, "Expected semicolon at the end of the expression.");
    return stmt;
}

static Statement let_statement(Parser *parser, Tokenizer *tokenizer) {
    Token identifier = consume(parser, tokenizer, TOKEN_IDENTIFIER, "Expected identifier after 'let'.");
    char *name = own_string_n(identifier.start, identifier.length);
  
    Expression initializer;
    if (match(parser, tokenizer, 1, TOKEN_EQUAL)) {
        initializer = expression(parser, tokenizer);
    }

#ifdef venom_debug
    print_expression(initializer);
    printf("\n");
#endif

    Statement stmt = { .kind = STMT_LET, .name = name, .exp = initializer };
    consume(parser, tokenizer, TOKEN_SEMICOLON, "Expected semicolon at the end of the statement.");
    return stmt;
}

static Statement assign_statement(Parser *parser, Tokenizer *tokenizer) {
    char *identifier = own_string_n(parser->previous.start, parser->previous.length);
    consume(parser, tokenizer, TOKEN_EQUAL, "Expected '=' after the identifier.");
    Expression initializer = expression(parser, tokenizer);
    Statement stmt = { .kind = STMT_ASSIGN, .name = identifier, .exp = initializer };
    consume(parser, tokenizer, TOKEN_SEMICOLON, "Expected ';' at the end of the statement.");
    return stmt;
}

static Statement statement(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_PRINT)) {
        return print_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_LET)) {
        return let_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_IDENTIFIER)) {
        return assign_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_LEFT_BRACE)) {
        return (Statement){ .kind = STMT_BLOCK, .stmts = block(parser, tokenizer) };
    } else {
        assert(0);
    }
}

static Statement parse_statement(Parser *parser, Tokenizer *tokenizer) {
    return statement(parser, tokenizer);
}

void parse(Parser *parser, Tokenizer *tokenizer, Statement_DynArray *stmts) {
    parser->had_error = false;
    advance(parser, tokenizer);
    while (parser->current.type != TOKEN_EOF) {
        dynarray_insert(stmts, parse_statement(parser, tokenizer));
    }
}
