#ifndef venom_parser_h
#define venom_parser_h

#include <stdbool.h>
#include "dynarray.h"
#include "tokenizer.h"

typedef enum {
    LITERAL,
    VARIABLE,
    UNARY,
    BINARY,
} ExpressionKind;

typedef struct BinaryExpression BinaryExpression;

typedef struct Expression {
    ExpressionKind kind;
    union {
        struct Expression *exp;
        BinaryExpression *binexp;
        double dval;
    } data;
    char *name;
    char *operator;
} Expression;

typedef struct BinaryExpression {
    Expression lhs;
    Expression rhs;    
} BinaryExpression;

typedef enum {
    STATEMENT_LET,
    STATEMENT_ASSIGN,
    STATEMENT_PRINT,
} StatementKind;

typedef struct Statement {
    StatementKind kind;
    char *name;
    Expression exp;
} Statement;

typedef struct {
    Token current;
    Token previous;
    bool had_error;
} Parser;

typedef DynArray(Statement) Statement_DynArray;

void parse(Parser *parser, Tokenizer *tokenizer, Statement_DynArray *stmts);
void free_ast(Expression e);

#endif