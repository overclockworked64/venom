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
    switch (stmt.kind) {
        case STMT_LET: {
            free(stmt.name);
            break;
        }
        case STMT_BLOCK: {
            for (size_t i = 0; i < stmt.stmts.count; ++i) {
                free_stmt(stmt.stmts.data[i]);
            }
            dynarray_free(&stmt.stmts);
            break;
        }
        case STMT_IF: {
            free_stmt(*stmt.then_branch);
            free(stmt.then_branch);

            if (stmt.else_branch != NULL) {
                free_stmt(*stmt.else_branch);
                free(stmt.else_branch);
            }

            break;
        }
        case STMT_WHILE: {
            free_stmt(*stmt.body);
            free(stmt.body);
            break;
        }
        default: break;
    }

    free_expression(stmt.exp);
}

void free_expression(Expression e) {
    switch (e.kind) {
        case EXP_LITERAL: return;
        case EXP_VARIABLE: free(e.name); break;
        case EXP_UNARY: {
            free_expression(*e.data.exp);
            free(e.data.exp);
            break;
        }
        case EXP_BINARY: {
            free_expression(e.data.binexp->lhs);
            free_expression(e.data.binexp->rhs);
            free(e.data.binexp);
            break;
        }
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
    return (Expression){
        .kind = EXP_LITERAL,
        .data.dval = strtod(parser->previous.start, NULL),
    };
}

static Expression variable(Parser *parser) {
    return (Expression){
        .kind = EXP_VARIABLE,
        .name = own_string_n(parser->previous.start, parser->previous.length)
    };
}

static Expression primary();
static Expression expression(Parser *parser, Tokenizer *tokenizer);
static Statement statement(Parser *parser, Tokenizer *tokenizer);

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

static Expression finish_call(Parser *parser, Tokenizer *tokenizer, Expression exp) {
    Expression_DynArray arguments = {0};
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            dynarray_insert(&arguments, expression(parser, tokenizer));
        } while (match(parser, tokenizer, 1, TOKEN_COMMA));
    }
    consume(
        parser, tokenizer,
        TOKEN_RIGHT_PAREN,
        "Expected ')' after expression."
    );
    return (Expression){
        .kind = EXP_CALL,
        .arguments = arguments,
        .name = exp.name,
    };
}

static Expression call(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = primary(parser, tokenizer);
    for (;;) {
        if (match(parser, tokenizer, 1, TOKEN_LEFT_PAREN)) {
            expr = finish_call(parser, tokenizer, expr);
        } else {
            break;
        }
    }
    return expr;
}

static Expression unary(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_MINUS)) {
        Expression *right = malloc(sizeof(Expression));
        *right = unary(parser, tokenizer);
        Expression result = { .kind = EXP_UNARY, .data.exp = right };
        return result;
    }
    return call(parser, tokenizer);
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
    consume(
        parser, tokenizer,
        TOKEN_RIGHT_PAREN,
        "Unmatched closing parentheses."
    );
    return exp;
}

static Statement_DynArray block(Parser *parser, Tokenizer *tokenizer) {
    Statement_DynArray stmts = {0};
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        dynarray_insert(&stmts, statement(parser, tokenizer));
    }
    consume(
        parser, tokenizer,
        TOKEN_RIGHT_BRACE,
        "Expected '}' at the end of the block."
    );
    return stmts;
}

static Expression primary(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_NUMBER)) {
        return number(parser);
    } else if (match(parser, tokenizer, 1, TOKEN_IDENTIFIER)) {
        return variable(parser);
    } else if (match(parser, tokenizer, 1, TOKEN_LEFT_PAREN)) {
        return grouping(parser, tokenizer);
    }
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
        case EXP_CALL: {
            for (size_t i = 0; i < e.arguments.count; ++i) {
                print_expression(e.arguments.data[i]);
            }
            printf("()");
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
    consume(
        parser, tokenizer,
        TOKEN_SEMICOLON,
        "Expected semicolon at the end of the expression."
    );
    return (Statement){
        .kind = STMT_PRINT,
        .exp = exp,
        .name = NULL
    };
}

static Statement let_statement(Parser *parser, Tokenizer *tokenizer) {
    Token identifier = consume(
        parser, tokenizer,
        TOKEN_IDENTIFIER,
        "Expected identifier after 'let'."
    );
    char *name = own_string_n(identifier.start, identifier.length);
    Expression initializer;
    if (match(parser, tokenizer, 1, TOKEN_EQUAL)) {
        initializer = expression(parser, tokenizer);
    }
#ifdef venom_debug
    print_expression(initializer);
    printf("\n");
#endif
    consume(
        parser, tokenizer,
        TOKEN_SEMICOLON,
        "Expected semicolon at the end of the statement."
    );
    return (Statement){
        .kind = STMT_LET,
        .name = name,
        .exp = initializer
    };
}

static Statement expression_statement(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = expression(parser, tokenizer);
    consume(
        parser, tokenizer,
        TOKEN_SEMICOLON,
        "Expected ';' after expression"
    );
    return (Statement){
        .kind = STMT_EXPR,
        .exp = expr,
    };
}

static Statement if_statement(Parser *parser, Tokenizer *tokenizer) {
    consume(
        parser, tokenizer,
        TOKEN_LEFT_PAREN,
        "Expected '(' after if."
    );
    Expression condition = expression(parser, tokenizer);
    consume(
        parser, tokenizer,
        TOKEN_RIGHT_PAREN,
        "Expected ')' after the condition."
    );

    Statement *then_branch = malloc(sizeof(Statement));
    Statement *else_branch = NULL;

    *then_branch = statement(parser, tokenizer);

    if (match(parser, tokenizer, 1, TOKEN_ELSE)) {
        else_branch = malloc(sizeof(Statement));
        *else_branch = statement(parser, tokenizer);
    }

    return (Statement){
        .kind = STMT_IF,
        .then_branch = then_branch,
        .else_branch = else_branch,
        .exp = condition,
    };
}

static Statement while_statement(Parser *parser, Tokenizer *tokenizer) {
    consume(
        parser, tokenizer,
        TOKEN_LEFT_PAREN,
        "Expected '(' after while."
    );
    Expression condition = expression(parser, tokenizer);
    consume(
        parser, tokenizer,
        TOKEN_RIGHT_PAREN,
        "Expected ')' after condition."
    );
    Statement stmt = {
        .kind = STMT_WHILE,
        .exp = condition,
        .body = malloc(sizeof(Statement)),
    };
    *stmt.body = statement(parser, tokenizer);
    return stmt;
 }

static Statement function_statement(Parser *parser, Tokenizer *tokenizer) {
    Token name = consume(
        parser, tokenizer,
        TOKEN_IDENTIFIER,
        "Expected identifier after 'fn'."
    );
    consume(
        parser, tokenizer,
        TOKEN_LEFT_PAREN,
        "Expected '(' after identifier."
    );
    String_DynArray parameters = {0};
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            Token parameter = consume(
                parser, tokenizer,
                TOKEN_IDENTIFIER,
                "Expected parameter name."
            );
            dynarray_insert(
                &parameters,
                own_string_n(parameter.start, parameter.length)
            );
        } while (match(parser, tokenizer, 1, TOKEN_COMMA));
    }
    consume(
        parser, tokenizer,
        TOKEN_RIGHT_PAREN,
        "Expected ')' after the parameter list."
    );
    consume(
        parser, tokenizer,
        TOKEN_LEFT_BRACE,
        "Expected '{' after the ')'."
    );
    return (Statement){
        .kind = STMT_FN,
        .name = own_string_n(name.start, name.length),
        .stmts = block(parser, tokenizer),
        .parameters = parameters,
    };
}

static Statement return_statement(Parser *parser, Tokenizer *tokenizer) {
    Expression expr = expression(parser, tokenizer);
    consume(
        parser, tokenizer,
        TOKEN_SEMICOLON,
        "Expected ';' after return."
    );
    return (Statement){
        .kind = STMT_RETURN,
        .exp = expr,
    };
}

static Statement statement(Parser *parser, Tokenizer *tokenizer) {
    if (match(parser, tokenizer, 1, TOKEN_PRINT)) {
        return print_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_LET)) {
        return let_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_LEFT_BRACE)) {
        return (Statement){ .kind = STMT_BLOCK, .stmts = block(parser, tokenizer) };
    } else if (match(parser, tokenizer, 1, TOKEN_IF)) {
        return if_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_WHILE)) {
        return while_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_FN)) {
        return function_statement(parser, tokenizer);
    } else if (match(parser, tokenizer, 1, TOKEN_RETURN)) {
        return return_statement(parser, tokenizer);
    } else {
        return expression_statement(parser, tokenizer);
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
