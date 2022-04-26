#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynarray.h"
#include "parser.h"
#include "tokenizer.h"

#define venom_debug

void free_ast(Expression e) {
    if (e.kind == LITERAL) return;
    else if (e.kind == STRING) free(e.data.sval);
    else {
        free_ast(e.data.binexp->lhs);
        free_ast(e.data.binexp->rhs);

        if (e.kind == ASSIGN) free(e.name);

        free(e.data.binexp);
    }
}

static void parse_error(Parser *parser, char *message) {
    parser->had_error = true;
    fprintf(stderr, "parse error: %s\n", message);
}

static void print_token(Token token) {
    printf("current token: Token { .type: ");

    switch (token.type) {
        case TOKEN_PRINT:       printf("TOKEN_PRINT"); break;
        case TOKEN_LET:         printf("TOKEN_LET"); break;
        case TOKEN_IDENTIFIER:  printf("TOKEN_IDENTIFIER"); break;
        case TOKEN_NUMBER:      printf("TOKEN_NUMBER"); break;
        case TOKEN_LEFT_PAREN:  printf("TOKEN_LEFT_PAREN"); break;
        case TOKEN_RIGHT_PAREN: printf("TOKEN_RIGHT_PAREN"); break;
        case TOKEN_STAR:        printf("TOKEN_STAR"); break;
        case TOKEN_SLASH:       printf("TOKEN_SLASH"); break;
        case TOKEN_PLUS:        printf("TOKEN_PLUS"); break;
        case TOKEN_MINUS:       printf("TOKEN_MINUS"); break;
        case TOKEN_DOT:         printf("TOKEN_DOT"); break;
        case TOKEN_SEMICOLON:   printf("TOKEN_SEMICOLON"); break;
        case TOKEN_EQUALS:      printf("TOKEN_EQUALS"); break;
        case TOKEN_EOF:         printf("TOKEN_EOF"); break;
        case TOKEN_ERROR:       printf("TOKEN_ERROR"); break;
        default: break;
    }

    printf(" , value: %.*s }\n", token.length, token.start);
}

static Token advance(Parser *parser, Tokenizer *tokenizer) {
    parser->previous = parser->current;
    parser->current = get_token(tokenizer);
#ifdef venom_debug
    print_token(parser->current);
#endif
    return parser->current;
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
    expr.kind = LITERAL,
    expr.data.dval = strtod(parser->previous.start, NULL);
    return expr;
}

static Expression string(Parser *parser) {
    Expression expr;
    expr.kind = STRING,
    expr.data.sval = malloc(255);
    snprintf(expr.data.sval, parser->previous.length + 1, "%s", parser->previous.start);
    return expr;
}

static Expression primary();

static ExpressionKind operator(Token token) {
    switch (token.type) {
        case TOKEN_PLUS:   return ADD;
        case TOKEN_MINUS:  return SUB;
        case TOKEN_STAR:   return MUL;
        case TOKEN_SLASH:  return DIV;
        case TOKEN_EQUALS: return ASSIGN;
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

static Expression assignment(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = term(parser, tokenizer);
    char *name = malloc(255);
    snprintf(name, parser->previous.length + 1, "%s", parser->previous.start);
    if (match(parser, tokenizer, 1, TOKEN_EQUALS)) {
        ExpressionKind kind = operator(parser->previous);
        Expression right = term(parser, tokenizer);
        Expression result = { .kind = kind, .data.binexp = malloc(sizeof(BinaryExpression)), .name = name };
        result.data.binexp->lhs = expr;
        result.data.binexp->rhs = right;
        expr = result;
    }
    return expr;
}

static Expression expression(Parser *parser, Tokenizer *tokenizer) {
    return assignment(parser, tokenizer);
}

static Expression grouping(Parser *parser, Tokenizer *tokenizer) {
    Expression exp = expression(parser, tokenizer);
    consume(parser, tokenizer, TOKEN_RIGHT_PAREN, "Unmatched closing parentheses.");
    return exp;
}

static Expression primary(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_NUMBER)) return number(parser);
    else if (match(parser, tokenizer, 1, TOKEN_IDENTIFIER)) return string(parser);
    else if (match(parser, tokenizer, 1, TOKEN_LEFT_PAREN)) return grouping(parser, tokenizer);
}

#ifdef venom_debug
static void print_expression(Expression e) {
    printf("(");
    
    if (e.kind == LITERAL) { 
        printf("%f", e.data.dval);
    } else if (e.kind == STRING) {
        printf("%s", e.data.sval);
    } else {
        print_expression(e.data.binexp->lhs);

        switch (e.kind) {
            case ADD:    printf(" + "); break;
            case SUB:    printf(" - "); break;
            case MUL:    printf(" * "); break;
            case DIV:    printf(" / "); break;
            case ASSIGN: printf(" = "); break;
            default:
                break;
        }

        print_expression(e.data.binexp->rhs);
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

    Statement stmt = { .kind = STATEMENT_PRINT, .exp = exp };
    consume(parser, tokenizer, TOKEN_SEMICOLON, "Expected semicolon at the end of the expression.");
    return stmt;
}

static Statement variable_declaration(Parser *parser, Tokenizer *tokenizer) {
    Expression exp = expression(parser, tokenizer);

#ifdef venom_debug
    print_expression(exp);
    printf("\n");
#endif

    Statement stmt = { .kind = STATEMENT_LET, .exp = exp };
    consume(parser, tokenizer, TOKEN_SEMICOLON, "Expected semicolor at the end of the statement.");
    return stmt;
}

static Statement statement(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_PRINT)) {
        return print_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_LET)) {
        return variable_declaration(parser, tokenizer);
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
